use log::info;

use crate::{I2CBus, I2cBusDevice};

pub struct Ds1338<'a> {
    i2c: I2cBusDevice<'a>,
}

impl Ds1338<'_> {
    pub const DEFAULT_ADDRESS: u8 = 0x68;

    pub fn bind(bus: &I2CBus, addr: u8) -> Ds1338<'_> {
        Ds1338 {
            i2c: I2cBusDevice::new(bus, addr),
        }
    }

    pub async fn send(&mut self, raw: &[u8]) {
        self.i2c.write(raw).await.unwrap();
    }

    pub async fn get(&mut self) {
        let mut r: [u8; 8] = [0; 8];
        self.i2c.write_read(&[0x00], &mut r).await.unwrap();

        // seconds
        let sec10 = (r[0] & 0b01110000) >> 4;
        let sec01 = r[0] & 0b1111;
        // minutes
        let min10 = (r[1] & 0b01110000) >> 4;
        let min01 = r[1] & 0b1111;
        // hours
        let hrs10 = (r[2] & 0b00110000) >> 4;
        let hrs01 = r[2] & 0b1111;

        info!(
            "RTC: {}{}:{}{}:{}{}",
            hrs10, hrs01, min10, min01, sec10, sec01
        );
    }
}
