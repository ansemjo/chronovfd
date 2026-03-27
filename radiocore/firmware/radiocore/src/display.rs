use core::ops::Add;

use jiff::{
    SignedDuration, ToSpan,
    civil::{DateTime, Time, time},
};

use crate::{I2CBus, I2cBusDevice};

pub struct VacuumDisplay<'a> {
    i2c: I2cBusDevice<'a>,
}

impl VacuumDisplay<'_> {
    pub const DEFAULT_ADDRESS: u8 = 0x42;

    pub fn bind(bus: &I2CBus, address: u8) -> VacuumDisplay<'_> {
        VacuumDisplay {
            i2c: I2cBusDevice::new(bus, address),
        }
    }

    pub async fn send(&mut self, raw: &[u8]) {
        self.i2c.write(raw).await.unwrap();
    }

    pub async fn brightness(&mut self, value: u8) {
        self.send(&[0x10, value]).await
    }

    pub async fn blank(&mut self) {
        self.send(&[0x7f]).await
    }
}

// this is an approximation to adjust the brightness for daytime
pub fn nightlight_curve(time: jiff::civil::DateTime, min: u8, max: u8) -> u8 {
    let (sunrise, sunset) = approximate_sunrise_sunset(time);
    let time = time.time();

    const SLOPE: SignedDuration = SignedDuration::from_hours(2);
    let sunrise_start = sunrise.saturating_sub(SLOPE / 2);
    let sunset_end = sunset.saturating_add(SLOPE / 2);

    if time <= sunrise_start || time >= sunset_end {
        // night
        return min;
    };
    match time.duration_since(sunrise_start) {
        // get brighter around sunrise
        morning if morning < SLOPE => {
            let f = morning.div_duration_f32(SLOPE);
            return (min as f32 + (max - min) as f32 * f) as u8;
        }
        _ => (),
    };
    match time.duration_until(sunset_end) {
        // get darker around sunset
        evening if evening < SLOPE => {
            let f = evening.div_duration_f32(SLOPE);
            return (max as f32 - (max - min) as f32 * f) as u8;
        }
        _ => (),
    };
    // daytime
    return max;
}

pub fn approximate_sunrise_sunset(dt: DateTime) -> (Time, Time) {
    // winter solstice: Dec 21 (sunrise 08:00, sunset 16:00)
    const WINTER: i16 = 354; // jiff::civil::date(2025, 12, 21).day_of_year()
    const WINTER_SUNRISE: Time = time(8, 0, 0, 0);
    const WINTER_SUNSET: Time = time(16, 0, 0, 0);
    // summer solstice: Jun 21 (sunrise 05:00, sunset 22:00)
    const SUMMER: i16 = 171; // jiff::civil::date(2025, 06, 21).day_of_year();
    const HALFYEAR: i16 = WINTER - SUMMER;

    // use days since winter solstice as year progress
    let since_winter = ((dt.day_of_year() - WINTER) + 365) % 365;
    let progress = if since_winter <= HALFYEAR {
        since_winter as f32 / HALFYEAR as f32
    } else {
        2.0 - (since_winter as f32 / HALFYEAR as f32)
    };

    // interpolate sunrise and sunset times
    let sunrise = WINTER_SUNRISE.add(((-3.0 * 60.0 * progress) as i16).minutes());
    let sunset = WINTER_SUNSET.add(((6.0 * 60.0 * progress) as i16).minutes());
    (sunrise, sunset)
}
