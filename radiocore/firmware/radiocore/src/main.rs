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
use log::{debug, info};

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
use radiocore::radio::{RDSUpdate, Si4706Radio, rds_to_julian};
use radiocore::rtc::{Ds1338, RtcControls, SqweRate};
use static_cell::StaticCell;

// Default app-descriptor required by the esp-idf bootloader
// https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/app_image_format.html#application-description
esp_bootloader_esp_idf::esp_app_desc!();

pub static SECONDS: Watch<CriticalSectionRawMutex, Instant, 2> = Watch::new();

pub static RDSTIME: Watch<CriticalSectionRawMutex, jiff::Zoned, 2> = Watch::new();

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

    // rtc.send(&[0, 0b0000_0000, 0b0010_0000, 0b0000_0001]).await; // set 01:20:00

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
    radio.preparations().await;
    radio.tune(10000).await;
    Timer::after_secs(1).await;
    let mut rds = [0; 14];
    let mut lastinfo = Instant::now();
    loop {
        if lastinfo.elapsed() > Duration::from_secs(20) {
            radio.receiver_status().await;
            lastinfo = Instant::now();
        }
        if let Some(instant) = radio.is_rds_ready(&mut rds).await {
            let block_a = u16::from_be_bytes([rds[4], rds[5]]);
            let block_b = u16::from_be_bytes([rds[6], rds[7]]);
            let block_c = u16::from_be_bytes([rds[8], rds[9]]);
            let block_d = u16::from_be_bytes([rds[10], rds[11]]);
            let b_group = (block_b & 0xf000) >> 12;
            let group = match b_group {
                0x00 => "STATION",
                0x02 => "MESSAGE",
                0x04 => "DATETIME",
                _ => &format!(2; "{:02x}", b_group).unwrap(),
            };
            if group == "DATETIME" {
                info!(
                    "RDS [{:04x}] typ: {}   C={:04x}, D={:04x}",
                    block_a, group, block_c, block_d
                );
                let dt = rds_to_julian(&rds);
                if let Some(dt) = radio.plausible_update(RDSUpdate::new(instant, dt)) {
                    rdstime.send(dt);
                }
            } else {
                debug!(
                    "RDS [{:04x}] typ: {}   C={:04x}, D={:04x}",
                    block_a, group, block_c, block_d
                );
            }
        }
        Timer::after_millis(10).await;
    }
    // radio.tune_status().await;
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
    rtc.set_controls(RtcControls {
        oscillator_stop: false,
        output_control: true,
        sqaurewave_output: true,
        rate_select: SqweRate::_32768kHz,
    })
    .await;

    let mut tick = ticker.receiver().unwrap();
    let mut timeupdate = rdstime.receiver().unwrap();

    loop {
        match select::select(tick.changed(), timeupdate.changed()).await {
            Either::First(_) => {
                let dt = rtc.get_time().await;
                let brightness = interpolate_brightness(dt, 63, 120);
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
