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
