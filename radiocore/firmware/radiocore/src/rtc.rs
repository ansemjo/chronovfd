use crate::{I2CBus, I2cBusDevice};

pub struct Ds1338<'a> {
    i2c: I2cBusDevice<'a>,
}

//  addr | bit 7 |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
// ------|-------|-------|-------|-------|-------|-------|-------|-------|
//    00 |  halt |      10  seconds      |            seconds            | 00-59
//    01 |   0   |      10  minutes      |            minutes            | 00-59
//    02 |   0   | 1: 12 | am/pm | 10 hr |             hour              |  1-12 + am/pm
//    .. |   0   | 0: 24 | 20 hr | 10 hr |             hour              | 00-23
//    03 |   0   |   0   |   0   |   0   |   0   |      day of week      |  1-7
//    04 |   0   |   0   |    10 date    |             date              | 01-31
//    05 |   0   |   0   |   0   | 10mon |            month              | 01-12
//    06 |         10  year              |             year              | 00-99
//    07 |  out  |   0   |  osf  |  sqwe |   0   |   0   |  rs1  |  rs0  | controls
// 08-3f |                                                               | 56b RAM
impl Ds1338<'_> {
    pub fn bind(bus: &I2CBus) -> Ds1338<'_> {
        Ds1338 {
            i2c: I2cBusDevice::new(bus, 0b1101000),
        }
    }

    pub async fn get_controls(&mut self) -> RtcControls {
        let mut r: [u8; 1] = [0; 1];
        self.i2c.write_read(&[0x07], &mut r).await.unwrap();
        RtcControls::from(r[0])
    }

    pub async fn get(&mut self) -> RtcState {
        let mut r: [u8; 8] = [0; 8];
        self.i2c.write_read(&[0x00], &mut r).await.unwrap();
        RtcState::from(&r)
    }

    pub async fn set_controls(&mut self, ctl: RtcControls) {
        self.i2c.write(&[0x07, ctl.into()]).await.unwrap();
    }

    pub async fn get_time(&mut self) -> jiff::civil::DateTime {
        self.get().await.datetime
    }

    pub async fn set_time(&mut self, dt: jiff::civil::DateTime) {
        self.i2c
            .write(&[
                0x00,
                bcd_encode(dt.second() as u8),
                bcd_encode(dt.minute() as u8),
                bcd_encode(dt.hour() as u8),
                bcd_encode(dt.weekday().to_monday_zero_offset() as u8),
                bcd_encode(dt.day() as u8),
                bcd_encode(dt.month() as u8),
                bcd_encode((dt.year() % 100) as u8),
            ])
            .await
            .unwrap();
    }

    pub async fn get_zoned(&mut self) -> jiff::Zoned {
        let state = self.get().await;
        state.datetime.to_zoned(jiff::tz::TimeZone::UTC).unwrap()
    }

    pub async fn set_zoned(&mut self, dt: jiff::Zoned) {
        let dt = dt.with_time_zone(jiff::tz::TimeZone::UTC);
        self.set_time(dt.datetime()).await;
    }
}

#[derive(Debug)]
pub struct RtcState {
    pub datetime: jiff::civil::DateTime,
    pub controls: RtcControls,
    pub clock_halt: bool,
    pub hour_mode: HourMode,
}

impl From<&[u8; 8]> for RtcState {
    fn from(r: &[u8; 8]) -> Self {
        let clock_halt = (r[0] & 0b1000_0000) != 0;
        let hour_mode = HourMode::from(r[2]);
        let controls = RtcControls::from(r[7]);
        // parse bcd datetime fields
        let seconds = bcd_decode(r[0] & 0b0111_1111);
        let minutes = bcd_decode(r[1]);
        let hours = match hour_mode {
            HourMode::_24Hours => bcd_decode(r[2] & 0b0011_1111),
            HourMode::_12Hours => {
                let mut hr = bcd_decode(r[2] & 0b0001_1111) % 12;
                if (r[2] & 0b0010_0000) != 0 {
                    hr += 12
                };
                hr
            }
        };
        let day = bcd_decode(r[4]); // date-day, not dow
        let month = bcd_decode(r[5]);
        let year = bcd_decode(r[6]);
        let datetime = jiff::civil::datetime(
            year as i16 + 2000,
            month.try_into().unwrap(),
            day.try_into().unwrap(),
            hours.try_into().unwrap(),
            minutes.try_into().unwrap(),
            seconds.try_into().unwrap(),
            0,
        );
        Self {
            hour_mode,
            clock_halt,
            controls,
            datetime,
        }
    }
}

fn bcd_decode(r: u8) -> u8 {
    let tens = (r & 0b11110000) >> 4;
    let ones = r & 0b1111;
    tens * 10 + ones
}

fn bcd_encode(n: u8) -> u8 {
    if n > 99 {
        panic!("can't encode larger than 99 as BCD")
    }
    let tens = (n / 10) << 4;
    let ones = n % 10;
    tens | ones
}

#[derive(Debug)]
pub enum HourMode {
    _12Hours = 0b0100_0000,
    _24Hours = 0b0000_0000,
}

impl From<u8> for HourMode {
    fn from(r: u8) -> Self {
        match (r & 0b0100_0000) != 0 {
            true => Self::_12Hours,
            false => Self::_24Hours,
        }
    }
}

#[derive(Debug)]
pub struct RtcControls {
    // output pin level when sqwe is disabled
    pub output_control: bool,
    // oscillator stop flag
    pub oscillator_stop: bool,
    // enable square wave output on pin
    pub sqaurewave_output: bool,
    // select SQWE rate
    pub rate_select: SqweRate,
}

impl From<u8> for RtcControls {
    fn from(r: u8) -> Self {
        Self {
            output_control: (r & 0b1000_0000) != 0,
            oscillator_stop: (r & 0b0010_0000) != 0,
            sqaurewave_output: (r & 0b0001_0000) != 0,
            rate_select: SqweRate::from(r),
        }
    }
}

impl Into<u8> for RtcControls {
    fn into(self) -> u8 {
        let mut val = 0 as u8;
        if self.output_control {
            val |= 0b1000_0000;
        }
        if self.oscillator_stop {
            val |= 0b0010_0000;
        }
        if self.sqaurewave_output {
            val |= 0b0001_0000;
        }
        match self.rate_select {
            SqweRate::_1Hz => (),
            SqweRate::_4096kHz => val |= 0b01,
            SqweRate::_8192kHz => val |= 0b10,
            SqweRate::_32768kHz => val |= 0b11,
        }
        val
    }
}

#[derive(Debug)]
pub enum SqweRate {
    _1Hz = 0b00,
    _4096kHz = 0b01,
    _8192kHz = 0b10,
    _32768kHz = 0b11,
}

impl From<u8> for SqweRate {
    fn from(r: u8) -> Self {
        match r & 0b11 {
            0b00 => Self::_1Hz,
            0b01 => Self::_4096kHz,
            0b10 => Self::_8192kHz,
            0b11 => Self::_32768kHz,
            _ => panic!("only values 0..3 allowed"),
        }
    }
}
