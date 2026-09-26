#![no_std]
#![no_main]
#![deny(
    clippy::mem_forget,
    reason = "mem::forget is generally not safe to do with esp_hal types, especially those \
    holding buffers for the duration of a data transfer."
)]
#![deny(clippy::large_stack_frames)]

use embassy_executor::Spawner;
use embassy_futures::select::{self, Either};
use embassy_sync::blocking_mutex::raw::CriticalSectionRawMutex;
use embassy_sync::mutex::Mutex;
use embassy_sync::watch::Watch;
use embassy_time::{Duration, Instant, Timer};
use esp_hal::Async;
use esp_hal::clock::CpuClock;
use esp_hal::gpio::AnyPin;
use esp_hal::i2c;
use esp_hal::interrupt::software::SoftwareInterruptControl;
use esp_hal::time::Rate;
use esp_hal::timer::timg::TimerGroup;
use heapless::format;
use log::{debug, info, warn};

use esp_backtrace as _;

#[unsafe(no_mangle)]
pub extern "C" fn custom_halt() -> ! {
    const RESET: Option<&'static str> = option_env!("ESP_BACKTRACE");
    if let Some(r) = RESET
        && r == "RESET"
    {
        // reset the chip on panic
        esp_hal::system::software_reset();
    }
    // otherwise drop to infinite loop
    loop {}
}

use radiocore::display::VacuumDisplay;
use radiocore::nightlight::interpolate_brightness;
use radiocore::radio::{RDSUpdate, RdsContent, Si4706Radio};
use radiocore::rtc::{Ds1338, RtcControls, SqweRate};
use static_cell::StaticCell;

// Default app-descriptor required by the esp-idf bootloader
// https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/app_image_format.html#application-description
esp_bootloader_esp_idf::esp_app_desc!();

pub static SECONDS: Watch<CriticalSectionRawMutex, Instant, 2> = Watch::new();

pub static RDSTIME: Watch<CriticalSectionRawMutex, jiff::Zoned, 2> = Watch::new();

pub static RESET_RTC: bool = false;

type I2cBus = Mutex<CriticalSectionRawMutex, i2c::master::I2c<'static, Async>>;

#[allow(
    clippy::large_stack_frames,
    reason = "it's not unusual to allocate larger buffers etc. in main"
)]
#[esp_rtos::main]
async fn main(spawner: Spawner) {
    // generator version: 1.2.0

    // use: $ ESP_LOG=Debug cargo run --release
    esp_println::logger::init_logger_from_env();

    // initialize to maximum frequency of 160 MHz
    let config = esp_hal::Config::default().with_cpu_clock(CpuClock::_160MHz);
    let peripherals = esp_hal::init(config);

    // prepare interrupt source for delay timers
    let timg0 = TimerGroup::new(peripherals.TIMG0);
    let sw_interrupt = SoftwareInterruptControl::new(peripherals.SW_INTERRUPT);
    esp_rtos::start(timg0.timer0, sw_interrupt.software_interrupt0);

    info!("Embassy initialized!");

    // initialize i2c controller
    let i2c = i2c::master::I2c::new(
        peripherals.I2C0,
        i2c::master::Config::default().with_frequency(Rate::from_khz(400)),
    )
    .unwrap()
    .with_sda(peripherals.GPIO5)
    .with_scl(peripherals.GPIO6);
    let i2c = i2c.into_async();

    // create a shared bus for multiple devices
    // https://github.com/embassy-rs/embassy/blob/main/examples/rp/src/bin/shared_bus.rs
    static I2CBUS: StaticCell<I2cBus> = StaticCell::new();
    let i2c = I2CBUS.init(Mutex::<CriticalSectionRawMutex, _>::new(i2c));

    spawner.must_spawn(radio(i2c, peripherals.GPIO3.into(), &RDSTIME));
    spawner.must_spawn(ticktock(i2c, &SECONDS, &RDSTIME));
    spawner.must_spawn(seconds_ticker());
}

#[embassy_executor::task]
pub async fn radio(
    bus: &'static I2cBus,
    enable_pin: AnyPin<'static>,
    rdstime: &'static Watch<CriticalSectionRawMutex, jiff::Zoned, 2>,
) {
    let mut radio = Si4706Radio::bind(bus, Si4706Radio::ADDRESS_SEN_HIGH, enable_pin);
    let rdstime = rdstime.sender();
    radio.power_up().await;
    radio.log_revision().await;

    let threshold_snr = 15;
    let threshold_rssi = 20;
    radio.preparations(threshold_snr, threshold_rssi).await;

    // radio.tune(10000).await; // R.SH
    // radio.tune(09995).await; // R.SH
    // radio.tune(08755).await; // NDR 2
    // radio.tune(09035).await; // NDR 90.3
    // radio.tune(09400).await; // BAD: no signal for testing
    // radio.tune(10365).await; // Radio Hamburg
    // radio.tune(09975).await; // Berlin: Antenne Brandenburg
    // radio.tune(10750).await; // Berlin: BB Radio // NO DateTime despite very good RDS
    // radio.tune(10455).await; // Berlin: RTL
    let best = radio.full_frequency_scan(false).await;
    radio.tune(best[0].freq).await;
    let fm = radio.tune_status(false, false).await;
    info!("Tuned to {:5.2} MHz", f32::from(fm.freq) / 100.0);
    Timer::after_secs(1).await;

    // channel quality tracking
    let mut rds_packet_count: u32 = 0;
    let mut datetime_packet_count: u32 = 0;
    let mut last_quality_check = Instant::now();
    let mut no_datetime_periods: u8 = 0;

    async fn reseek(radio: &mut Si4706Radio<'_>, periods: &mut u8) {
        radio.seek().await;
        let new_fm = radio.tune_status(false, false).await;
        info!(
            "Tuned to {:5.2} MHz with snr={:2}",
            f32::from(new_fm.freq) / 100.0,
            new_fm.snr
        );
        *periods = 0;
    }

    loop {
        // periodically check channel quality
        if last_quality_check.elapsed() > Duration::from_secs(60) {
            let fm = radio.tune_status(false, false).await;

            // Two-stage quality check:
            // 1. No RDS packets at all or SNR low -> channel is bad
            // 2. RDS packets received but no datetime -> still acceptable for a while
            //    (datetime packets come once per minute, might have errors)
            let received_rds = rds_packet_count > 0;
            let received_datetime = datetime_packet_count > 0;

            if fm.snr < threshold_snr || !received_rds {
                warn!(
                    "Channel {:5.2} MHz bad: snr={:2}, #rds={}, #datetime={}, seeking ...",
                    f32::from(fm.freq) / 100.0,
                    fm.snr,
                    rds_packet_count,
                    datetime_packet_count
                );
                reseek(&mut radio, &mut no_datetime_periods).await;
            } else if !received_datetime {
                no_datetime_periods += 1;
                if no_datetime_periods >= 10 {
                    // 10 periods * 60s = 10 minutes without datetime
                    warn!(
                        "Channel {:5.2} MHz bad: no datetime packets received, #rds={}, seeking...",
                        f32::from(fm.freq) / 100.0,
                        rds_packet_count
                    );
                    reseek(&mut radio, &mut no_datetime_periods).await;
                } else {
                    warn!(
                        "Channel {:5.2} MHz questionable: snr={:2}, #rds={}, #datetime={}",
                        f32::from(fm.freq) / 100.0,
                        fm.snr,
                        rds_packet_count,
                        datetime_packet_count
                    );
                }
            } else {
                // channel is good: receives RDS and datetime packets
                no_datetime_periods = 0;
                info!(
                    "Channel {:5.2} MHz good: snr={:2}, #rds={}, #datetime={}",
                    f32::from(fm.freq) / 100.0,
                    fm.snr,
                    rds_packet_count,
                    datetime_packet_count
                );
            }

            // reset counters for next quality check
            last_quality_check = Instant::now();
            rds_packet_count = 0;
            datetime_packet_count = 0;
        }

        if let Some(packet) = radio.is_rds_ready().await {
            rds_packet_count += 1;

            match packet.content {
                RdsContent::DateTime(datetime) => {
                    datetime_packet_count += 1;
                    if let Some(dt) =
                        radio.plausible_update(RDSUpdate::new(packet.instant, datetime))
                    {
                        rdstime.send(dt);
                    }
                }
                _ => {}
            }
            Timer::after_millis(5).await;
        } else {
            Timer::after_millis(2).await;
        }
    }
}

#[embassy_executor::task]
pub async fn ticktock(
    bus: &'static I2cBus,
    ticker: &'static Watch<CriticalSectionRawMutex, Instant, 2>,
    rdstime: &'static Watch<CriticalSectionRawMutex, jiff::Zoned, 2>,
) {
    let mut vfd = VacuumDisplay::bind(bus, VacuumDisplay::DEFAULT_ADDRESS);
    vfd.brightness(100).await;

    let mut rtc = Ds1338::bind(bus);
    info!("RTC: {:?}", rtc.get().await);
    // rtc.clear_ram().await;
    info!("RTC MEMORY: {:x?}", rtc.get_nvram().await);
    rtc.set_controls(RtcControls {
        oscillator_stop: false,
        output_control: true,
        sqaurewave_output: true,
        rate_select: SqweRate::_32768kHz,
    })
    .await;

    // remove to actually retain rtc data
    if RESET_RTC {
        let reset = jiff::civil::date(2000, 01, 01).at(00, 00, 00, 0);
        rtc.set_time(reset).await;
    }

    let mut tick = ticker.receiver().unwrap();
    let mut timeupdate = rdstime.receiver().unwrap();

    loop {
        match select::select(tick.changed(), timeupdate.changed()).await {
            Either::First(_) => {
                let dt = rtc.get_time().await;
                let brightness = interpolate_brightness(dt, 64, 120);
                debug!("set brightness {}", brightness);
                vfd.brightness(brightness).await;
                let mut str = format!(5; "{:02}:{:02}", dt.hour(), dt.minute())
                    .unwrap()
                    .into_bytes();
                if dt.second() % 2 == 0 {
                    str[2] = b' ';
                }
                vfd.send(&str).await;
            }
            Either::Second(upd) => {
                rtc.set_time(upd.datetime()).await;
            }
        }
    }
}

#[embassy_executor::task]
async fn seconds_ticker() {
    let mut timer = embassy_time::Ticker::every(Duration::from_hz(1));
    let watch = SECONDS.sender();
    info!("starting ticker");
    loop {
        timer.next().await;
        let now = Instant::now();
        watch.send(now);
        debug!(
            "uptime {:03}s {:06}",
            now.as_secs(),
            now.as_micros() % 1000000
        );
    }
}
