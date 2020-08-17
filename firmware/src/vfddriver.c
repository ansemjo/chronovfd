#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/timer.h"

#include "vfddriver.h"
#include "segments.h"

// ------------------------ spi driver ------------------------

// set strobe low before transaction
void vfd_spi_strobe_low(spi_transaction_t *t) {
  gpio_set_level(((vfd_handle_t*)t->user)->pin.strobe, 0);
}

// set strobe high after transaction to latch contents
void vfd_spi_strobe_high(spi_transaction_t *t) {
  gpio_set_level(((vfd_handle_t*)t->user)->pin.strobe, 1);
}

// transmit data to a hv5812 vfd driver
void vfd_spi_data(vfd_handle_t *vfd, uint16_t data) {

  // prepare a new transaction
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));

  // embed vfd handle for context
  t.user = vfd;

  // prepare data to be sent, 20 bits fully define all output pins
  uint32_t buf = SPI_SWAP_DATA_TX(data & (GRIDMASK|SEGMENTMASK), 20);
  t.tx_buffer = &buf;
  t.length = 20;

  // transmit and wait for transaction to finish
  ESP_ERROR_CHECK(spi_device_transmit(vfd->spi, &t));

}

vfd_handle_t* vfd_init(vfd_pin_t pin, char* tag) {

  // allocate new handle on heap
  vfd_handle_t *vfd = calloc(1, sizeof(vfd_handle_t));
  vfd->pin = pin;
  vfd->tag = tag;

  // configure the spi bus
  spi_bus_config_t buscfg = {
    .sclk_io_num = pin.clock,
    .mosi_io_num = pin.data,
    .miso_io_num = -1,
    .quadhd_io_num = -1,
    .quadwp_io_num = -1,
  };
  ESP_ERROR_CHECK(spi_bus_initialize(pin.host, &buscfg, 0));
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
  ESP_ERROR_CHECK(spi_bus_add_device(pin.host, &devcfg, &vfd->spi));
  ESP_LOGI(tag, "device added; strobe=%d", pin.strobe);

  // configure auxiliary gpios for display driver
  gpio_set_direction(pin.strobe, GPIO_MODE_OUTPUT);
  gpio_set_level(pin.strobe, 0); // don't latch now
  gpio_set_direction(pin.blank, GPIO_MODE_OUTPUT);
  gpio_set_level(pin.blank, 0); // don't blank
  gpio_set_direction(pin.fil_shdn, GPIO_MODE_OUTPUT);
  gpio_set_level(pin.fil_shdn, 0); // filament supply on
  gpio_set_direction(pin.hv_shdn, GPIO_MODE_OUTPUT);
  gpio_set_level(pin.hv_shdn, 1); // hv supply on
  gpio_set_direction(pin.enable, GPIO_MODE_OUTPUT);
  gpio_set_level(pin.enable, 0); // enable the level shifter

  return vfd;

}

// ------------------------ digitmux timer ------------------------

uint16_t vfd_grids[5] = { G1, G2, G3, G4, G5 };

void vfd_digitmux(void *arg) {
  vfd_handle_t *vfd = arg;
  int p = vfd->pos;
  uint16_t data = (vfd->buf[p] & SEGMENTMASK) | vfd_grids[p];
  vfd_spi_data(vfd, data);
  vfd->pos = (p + 1) % 5;
}

void vfd_mux_init(vfd_handle_t *vfd, uint64_t period_us) {

  vfd->pos = 0;
  esp_timer_create_args_t muxargs = {
    .name = vfd->tag,
    .callback = vfd_digitmux,
    .arg = vfd,
    .dispatch_method = ESP_TIMER_TASK,
  };
  ESP_ERROR_CHECK(esp_timer_create(&muxargs, &vfd->mux));
  ESP_ERROR_CHECK(esp_timer_start_periodic(vfd->mux, period_us));

}

// ------------------------ convenience ------------------------

// convenience function to init defaults on chronovfd
vfd_handle_t* chronovfd_init() {

  vfd_pin_t defaults = {
    .host      = VFD_SPI_HOST,
    .enable    = VFD_PIN_ENABLE,
    .clock     = VFD_PIN_CLOCK,
    .data      = VFD_PIN_DATA,
    .strobe    = VFD_PIN_STROBE,
    .blank     = VFD_PIN_BLANK,
    .fil_shdn  = VFD_PIN_FILSHDN,
    .hv_shdn   = VFD_PIN_HVSHDN,
  };
  vfd_handle_t *vfd = vfd_init(defaults, "chronovfd");
  vfd_mux_init(vfd, 3000);
  vfd->buf[0] = segment_lookup('c');
  vfd->buf[1] = segment_lookup('r');
  vfd->buf[3] = segment_lookup('o');
  vfd->buf[4] = segment_lookup('n');

  return vfd;

}