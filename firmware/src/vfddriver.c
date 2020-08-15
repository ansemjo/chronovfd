#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/timer.h"

#include "vfddriver.h"
#include "segments.h"

// access a vfddriver_pin_t inside a user object of a spi transaction
#define spipin(t) ((vfd_handle_t*)t->user)->pin

// set strobe low before transaction
void vfd_spi_strobe_low(spi_transaction_t *t) {
  gpio_set_level(spipin(t).strobe, 0);
}

// set strobe high after transaction to latch contents
void vfd_spi_strobe_high(spi_transaction_t *t) {
  gpio_set_level(spipin(t).strobe, 1);
}

// transmit data to a hv5812 vfd driver
void vfd_data(vfd_handle_t *vfd, uint16_t data) {

  // prepare a new transaction
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  t.user = vfd; // embed vfd handle for context
  uint32_t buf = SPI_SWAP_DATA_TX(data & (GRIDS|SEGMENTS), 20);
  t.tx_buffer = &buf;
  t.length = 20;

  // transmit and wait for transaction
  esp_err_t ret = spi_device_polling_transmit(vfd->spidev, &t);
  ESP_ERROR_CHECK(ret);

}

vfd_handle_t vfd_init(spi_host_device_t host, vfd_pins_t pin, char* tag) {

  // configure the spi bus
  spi_bus_config_t buscfg = {
    .sclk_io_num = pin.clock,
    .mosi_io_num = pin.data,
    .miso_io_num = -1,
    .quadhd_io_num = -1,
    .quadwp_io_num = -1,
  };
  ESP_ERROR_CHECK(spi_bus_initialize(host, &buscfg, 0));
  ESP_LOGI(tag, "bus configured; clock=%d, data=%d", pin.clock, pin.data);
  
  // add the device on the bus
  spi_device_interface_config_t devcfg = {
    .clock_speed_hz = 5*1000*1000, // hv5812 maximum @ 125°C, 5V
    .mode = 0,
    .pre_cb  = vfd_spi_strobe_low,
    .post_cb = vfd_spi_strobe_high,
    .command_bits = 0,
    .address_bits = 0,
    .dummy_bits = 0,
    .duty_cycle_pos = 128,
    .spics_io_num = -1,
    .queue_size=1,
  };
  spi_device_handle_t spi;
  ESP_ERROR_CHECK(spi_bus_add_device(host, &devcfg, &spi));
  ESP_LOGI(tag, "device added; strobe=%d", pin.strobe);

  // configure auxiliary gpios for display driver
  gpio_set_direction(pin.strobe, GPIO_MODE_OUTPUT);
  gpio_set_level(pin.strobe, 0); // don't latch
  gpio_set_direction(pin.blank, GPIO_MODE_OUTPUT);
  gpio_set_level(pin.blank, 0); // don't blank
  gpio_set_direction(pin.fil_shdn, GPIO_MODE_OUTPUT);
  gpio_set_level(pin.fil_shdn, 0); // filament supply on
  gpio_set_direction(pin.hv_shdn, GPIO_MODE_OUTPUT);
  gpio_set_level(pin.hv_shdn, 1); // hv supply on
  gpio_set_direction(pin.enable, GPIO_MODE_OUTPUT);
  gpio_set_level(pin.enable, 0); // enable the level shifter

  vfd_handle_t vfd = { .pin = pin, .spidev = spi };
  return vfd;

}


// // ------------------------ TIMER ------------------------
#define HVMUX_GROUP   0
#define HVMUX_ISRG    TIMERG0
#define HVMUX_IDX     0
#define HVMUX_DIVIDER 80



// volatile bool MUXON = true;
// void hv_mux(void *null) {
//   HVMUX_ISRG.int_clr_timers.t0 = 1;
//   HVMUX_ISRG.hw_timer[HVMUX_IDX].config.alarm_en = TIMER_ALARM_EN;
//   MUXON = !MUXON;
//   gpio_set_level(HV_PIN_HVSHDN, MUXON);
// }

// void app_main() {

//   esp_err_t ret;
    
//   // init default nonvolatile storage
//   ret = nvs_flash_init();
//   if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//     ESP_ERROR_CHECK(nvs_flash_erase());
//     ret = nvs_flash_init();
//   }
//   ESP_ERROR_CHECK(ret);

  

//   // initialise multiplexing timer
//   timer_config_t hvmuxcfg = {
//     .divider = HVMUX_DIVIDER,
//     .counter_en = false,
//     .counter_dir = TIMER_COUNT_UP,
//     .auto_reload = TIMER_AUTORELOAD_EN,
//     .alarm_en = true,
//     .intr_type = TIMER_INTR_LEVEL,
//   };
//   timer_init(HVMUX_GROUP, HVMUX_IDX, &hvmuxcfg);
//   timer_set_counter_value(HVMUX_GROUP, HVMUX_IDX, 0);
//   timer_set_alarm_value(HVMUX_GROUP, HVMUX_IDX, 1 * (TIMER_BASE_CLK / HVMUX_DIVIDER));
//   timer_enable_intr(HVMUX_GROUP, HVMUX_IDX);
//   timer_isr_register(HVMUX_GROUP, HVMUX_IDX, &hv_mux, NULL, 0, NULL);
//   timer_start(HVMUX_GROUP, HVMUX_IDX);

//   led_init();
//   ESP_LOGI("LED", "turned on");


// }

