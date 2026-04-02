use embassy_time::Timer;
use esp_hal::gpio::{AnyPin, Level, Output, OutputConfig};
use log::{info, warn};

use crate::{I2CBus, I2cBusDevice};

pub struct Si4706Radio<'a> {
    i2c: I2cBusDevice<'a>,
    enable: Output<'a>,
    updates: heapless::HistoryBuf<RDSUpdate, 5>,
}

#[derive(Debug)]
pub struct RDSUpdate {
    instant: embassy_time::Instant,
    datetime: jiff::Zoned,
}

impl RDSUpdate {
    pub fn new(instant: embassy_time::Instant, datetime: jiff::Zoned) -> Self {
        RDSUpdate { instant, datetime }
    }
}

impl Si4706Radio<'_> {
    // addresses defined in Si4706-D50.pdf, § 4.15.1
    pub const ADDRESS_SEN_LOW: u8 = 0b0010001;
    pub const ADDRESS_SEN_HIGH: u8 = 0b1100011;

    pub fn bind<'a>(bus: &'a I2CBus, address: u8, enable_pin: AnyPin<'a>) -> Si4706Radio<'a> {
        Si4706Radio {
            i2c: I2cBusDevice::new(bus, address),
            enable: Output::new(enable_pin, Level::Low, OutputConfig::default()),
            updates: heapless::HistoryBuf::new(),
        }
    }

    // useful command bytes
    const POWER_UP: u8 = 0x01;
    const POWER_DOWN: u8 = 0x11;
    const GET_REV: u8 = 0x10;
    const SET_PROPERTY: u8 = 0x12;
    const GET_PROPERTY: u8 = 0x13;
    const GET_INT_STATUS: u8 = 0x14;
    const FM_TUNE_FREQ: u8 = 0x20;
    const FM_SEEK_START: u8 = 0x21;
    const FM_TUNE_STATUS: u8 = 0x22;
    const FM_RSQ_STATUS: u8 = 0x23;
    const FM_RDS_STATUS: u8 = 0x24;

    pub async fn power_up(&mut self) {
        // timing from Si4706-D50.pdf, Table 4
        Timer::after_micros(100).await;
        self.enable.set_high();
        Timer::after_nanos(30).await;
        // AN332.pdf, § 5.1.1
        let config = 0b00010000; // XOSCEN;
        self.i2c
            .write(&[Self::POWER_UP, config, 0x00])
            .await
            .unwrap();
        self.wait_for_cts().await;
        log::info!("radio powered up");
    }

    pub async fn power_down(&mut self) {
        self.i2c.write(&[Self::POWER_DOWN]).await.unwrap();
        self.enable.set_low();
    }

    pub async fn log_revision(&mut self) {
        // write command, then wait for cts
        self.i2c.write(&[Self::GET_REV]).await.unwrap();
        self.wait_for_cts().await;
        // read 16 bytes
        let mut readbuf: [u8; 16] = [0; 16];
        self.i2c.read(&mut readbuf).await.unwrap();
        log::info!(
            "Si47{:02x} fw: {}.{}, hw: {}.{} ({})",
            // part number
            readbuf[1],
            // firmware major.minor
            readbuf[2] as char,
            readbuf[3] as char,
            // component major.minor
            readbuf[6] as char,
            readbuf[7] as char,
            // chip revision
            readbuf[8] as char
        );
        self.wait_for_cts().await;
    }

    pub async fn preparations(&mut self) {
        log::debug!("FM_DEEMPHASIS: 50µs (EU)");
        self.set_property(0x1100, 0b01).await; // 10: 75µs (USA), 01: 50µs (EU, Japan)
        log::debug!("FM_MAX_TUNE_ERROR: 40 kHz");
        self.set_property(0x1108, 40).await; // in kHz
        log::debug!("FM_ANTENNA_INPUT: use LPI pin with embedded antenna");
        self.set_property(0x1107, 1).await; // 0: FMI, 1: TXO/LPI
        log::debug!("FM_SEEK_BAND_BOTTOM: 87.5 MHz");
        self.set_property(0x1400, 8750).await; // 64..108
        log::debug!("FM_SEEK_BAND_TOP: 108 MHz");
        self.set_property(0x1401, 10800).await; // 64..108
        log::debug!("FM_SEEK_FREQ_SPACING: 50 kHz");
        self.set_property(0x1402, 5).await; // in 10 kHz; allowed 50, 100, or 200

        let threshold_snr = 12;
        log::debug!("FM_SEEK_TUNE_SNR_THRESHOLD: {} dB", threshold_snr);
        self.set_property(0x1403, threshold_snr).await;
        let threshold_rssi = 22;
        log::debug!("FM_SEEK_TUNE_RSSI_THRESHOLD: {} dBµV", threshold_rssi);
        self.set_property(0x1404, threshold_rssi).await;

        log::debug!("FM_RDS_INT_SOURCE: on data receive in FIFO");
        self.set_property(0x1500, 0x0001).await;
        log::debug!("FM_RDS_INT_FIFO_COUNT: 4 groups for interrupt");
        self.set_property(0x1501, 4).await; // 0..25
        log::debug!("FM_RDS_CONFIG: enable RDS, allow only correctable data blocks");
        self.set_property(0x1502, 0b11_01_10_10_00000001).await; // 0: no err, 1: 1-2, 2: 3-5 corrected, 3: allow all
        log::debug!("FM_RDS_CONFIDENCE: require higher confidence");
        self.set_property(0x1503, 0x2222).await; // default: 0x1111
    }

    pub async fn set_property(&mut self, prop: u16, value: u16) {
        let props = prop.to_be_bytes();
        let value = value.to_be_bytes();
        self.i2c
            .write(&[
                Self::SET_PROPERTY,
                0x00,
                props[0],
                props[1],
                value[0],
                value[1],
            ])
            .await
            .unwrap();
        self.wait_for_cts().await;
    }

    pub async fn get_property(&mut self, prop: u16) -> u16 {
        let prop = prop.to_be_bytes();
        let mut readvalue: [u8; _] = [0; 4];
        self.i2c
            .write(&[Self::GET_PROPERTY, 0x00, prop[0], prop[1]])
            .await
            .unwrap();
        self.wait_for_cts().await;
        self.i2c.read(&mut readvalue).await.unwrap();
        u16::from_be_bytes(readvalue[2..4].try_into().unwrap())
    }

    pub async fn seek(&mut self) {
        log::info!("SEEK to next channel ...");
        self.i2c
            .write(&[Self::FM_SEEK_START, 0b1100])
            .await
            .unwrap();
        Timer::after_millis(100).await;
        self.wait_for_stc().await;
        self.tune_status(false, true).await;
    }

    pub async fn tune(&mut self, freq: u16) {
        log::info!("TUNE to {:3.2} ...", f32::from(freq) / 100.0);
        let freq = freq.to_be_bytes();
        let antcap = 0x00; // automatic
        self.i2c
            .write(&[Self::FM_TUNE_FREQ, 0x00, freq[0], freq[1], antcap])
            .await
            .unwrap();
        Timer::after_millis(100).await;
        self.wait_for_cts().await;
    }

    pub async fn tune_status(&mut self, cancel: bool, intack: bool) {
        let mut response = [0; 8];
        let flags = (cancel as u8) << 1 | (intack as u8);
        self.i2c
            .write_read(&[Self::FM_TUNE_STATUS, flags], &mut response)
            .await
            .unwrap();
        // TODO: return frequency info etc.
        log::info!("tune status: {:?}", response);
        self.wait_for_cts().await;
    }

    pub async fn receiver_status(&mut self) {
        let mut response = [0; 8];
        self.i2c
            .write_read(&[Self::FM_RSQ_STATUS, 0x00], &mut response)
            .await
            .unwrap();
        let valid = (response[2] & 1) != 0;
        let rssi = response[4];
        let snr = response[5];
        log::info!(
            "fm recevier: valid: {:?}, rssi: {:3} dBµV, snr: {:3} dB",
            valid,
            rssi,
            snr
        );
        self.wait_for_cts().await;
    }

    // wait for "clear to send"
    async fn wait_for_cts(&mut self) {
        let mut buf = [0; 1];
        loop {
            self.i2c.read(&mut buf).await.unwrap();
            log::debug!("wait for CTS: {:?}", status(&buf[0]));
            if status(&buf[0]).clear_to_send {
                return;
            }
            Timer::after_millis(10).await;
        }
    }

    // wait for "seek/tune complete"
    async fn wait_for_stc(&mut self) {
        let mut buf = [0; 1];
        loop {
            self.wait_for_cts().await;
            self.i2c.write(&[Self::GET_INT_STATUS]).await.unwrap();
            self.i2c.read(&mut buf).await.unwrap();
            log::debug!("wait for STC: {:?}", buf);
            if status(&buf[0]).seek_tune_complete {
                return;
            }
            Timer::after_millis(10).await;
        }
    }

    pub async fn is_rds_ready(&mut self, rds: &mut [u8]) -> Option<embassy_time::Instant> {
        let mut buf = [0; 1];
        let now = embassy_time::Instant::now();
        self.i2c.write(&[Self::GET_INT_STATUS]).await.unwrap();
        self.i2c.read(&mut buf).await.unwrap();
        if status(&buf[0]).rds_data {
            self.wait_for_cts().await;
            self.i2c.write(&[Self::FM_RDS_STATUS, 0x01]).await.unwrap();
            self.i2c.read(rds).await.unwrap();
            // log::info!("[RDS] {:08b} {:08b}", rds[1], rds[2]);
            if (rds[1] & 0x01) != 0 && (rds[2] & 0x01) != 0 {
                // rds_recv and rds is in sync
                return Some(now);
            }
        }
        None
    }

    // allowed drift of RDS updates in seconds
    const PLAUSIBLE_ERROR: i64 = 10;

    pub fn plausible_update(&mut self, now: RDSUpdate) -> Option<jiff::Zoned> {
        info!("checking update: {:?}", now.datetime);
        let length = self.updates.len();
        if length < 2 {
            // can't do a majority vote yet, just trust it
            warn!("not enough updates in buffer, can't do majority vote");
            self.updates.write(now);
            return Some(self.updates.recent().unwrap().datetime.clone());
        }

        let mut plausible: usize = 0;
        for then in self.updates.iter() {
            // calculate deltas and drift in seconds
            let d_instant = now.instant.duration_since(then.instant).as_secs() as i64;
            let d_timestamp = (&now.datetime - &then.datetime)
                .total(jiff::Unit::Second)
                .unwrap() as i64;
            let drift = d_instant - d_timestamp;
            // plausible if absolute drift is smaller than allowed error
            info!(
                "drift from {:?} ({:?}s ago): {}s",
                then.datetime, d_instant, drift
            );
            if drift.abs() < Self::PLAUSIBLE_ERROR {
                plausible += 1;
            }
        }
        // add update after checking, whatever the result
        self.updates.write(now);
        // if the majority was plausible, return the date
        if plausible > (length / 2) {
            info!("update is plausible ({} / {})!", plausible, length);
            return Some(self.updates.recent().unwrap().datetime.clone());
        }
        warn!("discard this update, implausible drift!");
        None
    }
}

#[derive(Debug)]
struct STATUS {
    clear_to_send: bool,
    // error: bool,
    // recv_signal_quality: bool,
    rds_data: bool,
    seek_tune_complete: bool,
}

fn status(byte: &u8) -> STATUS {
    return STATUS {
        clear_to_send: (byte & 1 << 7) != 0,
        // error: (byte & 1 << 6) != 0,
        // recv_signal_quality: (byte & 1 << 3) != 0,
        rds_data: (byte & 1 << 2) != 0,
        seek_tune_complete: (byte & 1 << 0) != 0,
    };
}

pub fn rds_to_julian(r: &[u8]) -> jiff::Zoned {
    // collect payload bits; "half block 2" through "block 4" are rds[7] to rds[11]
    // https://en.wikipedia.org/wiki/Radio_Data_System#Group_type_4_%E2%80%93_Version_A_%E2%80%93_Clock_time_and_date
    // https://en.wikipedia.org/wiki/Julian_day#Variants
    let mut julian = (((r[7] as u32) & 0b11) << 15) + ((r[8] as u32) << 7) + ((r[9] as u32) >> 1);
    let mut hours = (((r[9] & 0x01) << 4) + ((r[10] & 0xf0) >> 4)) as i8;
    let mut mins = (((r[10] & 0x0f) << 2) + ((r[11] & 0xc0) >> 6)) as i8;
    // offset is half hours!
    let offset = (if (r[11] & 0x10) != 0 { -1 } else { 1 }) * (r[11] & 0x0f) as i8;

    // calculate the proper time with offset
    // based on Konrad Kosmatka's librdsparser code in:
    // https://github.com/kkonradpl/librdsparser/blob/master/src/ct.c
    mins += (offset % 2) * 30;
    match mins {
        m if m >= 60 => hours += 1,
        m if m < 0 => hours -= 1,
        _ => (),
    }
    mins = mins % 60;
    hours += offset / 2;
    match hours {
        h if h >= 24 => julian += 1,
        h if h < 0 => julian -= 1,
        _ => (),
    }
    hours = hours % 24;

    // calculate year-month-day from modified julian date
    let year = (julian * 100 - 1507820) / 36525;
    let year_tmp = (year * 36525) / 100;
    let month = ((julian * 100 - 1495610) - year_tmp * 100) * 100 / 306001;
    let month_tmp = month * 306001 / 10000;
    let day = (julian - 14956 - year_tmp - month_tmp) as i8;
    let k = if month == 14 || month == 15 { 1 } else { 0 };
    let year = (1900 + year + k) as i16;
    let month = (month - 1 - k * 12) as i8;
    log::info!(
        "parsed RDS datetime: {year:04}-{month:02}-{day:02} {hours:02}:{mins:02} ({:.1})",
        f32::from(offset) / 2.0
    );

    // return the parsed date as jiff datetime
    let dt = jiff::civil::datetime(year, month, day, hours, mins, 0, 0);
    let tz = jiff::tz::Offset::from_seconds(offset as i32 * 30 * 60).unwrap();
    dt.to_zoned(tz.to_time_zone()).unwrap()
}
