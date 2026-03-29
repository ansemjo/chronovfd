#![no_std]

pub mod display;
pub mod nightlight;
pub mod radio;
pub mod rtc;

use embassy_embedded_hal::shared_bus::{I2cDeviceError, asynch::i2c::I2cDevice};
use embassy_sync::{blocking_mutex::raw::CriticalSectionRawMutex, mutex::Mutex};
use embedded_hal_async::i2c::I2c as _;
use esp_hal::i2c::master::{Error, I2c};

type I2CBusMuxDevice<'a> = I2cDevice<'a, CriticalSectionRawMutex, I2c<'static, esp_hal::Async>>;
type I2CBus = Mutex<CriticalSectionRawMutex, I2c<'static, esp_hal::Async>>;

pub struct I2cBusDevice<'a> {
    device: I2CBusMuxDevice<'a>,
    address: u8,
}

impl I2cBusDevice<'_> {
    pub fn new(bus: &I2CBus, address: u8) -> I2cBusDevice<'_> {
        I2cBusDevice {
            address,
            device: I2cDevice::new(bus),
        }
    }

    pub async fn write(&mut self, buf: &[u8]) -> Result<(), I2cDeviceError<Error>> {
        self.device.write(self.address, buf).await
    }

    pub async fn read(&mut self, buf: &mut [u8]) -> Result<(), I2cDeviceError<Error>> {
        self.device.read(self.address, buf).await
    }

    pub async fn write_read(
        &mut self,
        write: &[u8],
        read: &mut [u8],
    ) -> Result<(), I2cDeviceError<Error>> {
        self.device.write_read(self.address, write, read).await
    }
}
