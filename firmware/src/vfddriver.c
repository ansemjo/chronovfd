#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <driver/timer.h>

#include "vfddriver.h"
#include "segments.h"

// ------------------------ spi driver ------------------------

// set strobe low before spi transaction
void vfd_spi_strobe_low(spi_transaction_t *t) {
  gpio_set_level(((vfd_handle_t*)t->user)->pin.strobe, 0);
}

// set strobe high after spi transaction to latch contents
void vfd_spi_strobe_high(spi_transaction_t *t) {
  gpio_set_level(((vfd_handle_t*)t->user)->pin.strobe, 1);
}

// transmit data to a hv5812 vfd driver
void vfd_spi_data(vfd_handle_t *vfd, uint16_t data) {

  // prepare a new transaction
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  t.user = vfd;

  // prepare data to be sent, 20 bits fully define all output pins
  uint32_t buf = SPI_SWAP_DATA_TX(data & (GRIDMASK|SEGMENTMASK), 20);
  t.tx_buffer = &buf;
  t.length = 20;

  // transmit and wait for transaction to finish
  ESP_ERROR_CHECK(spi_device_polling_transmit(vfd->spi, &t));

}

// initialize spi interface to hv5812 display driver with auxiliary pins
vfd_handle_t* vfd_init(vfd_pin_t pin, char* tag) {

  // allocate new handle on heap
  vfd_handle_t *vfd = calloc(1, sizeof(vfd_handle_t));
  vfd->pin = pin;
  vfd->tag = tag; // TODO: better user string copy?

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
    // HV5812 maximum rated frequency @ 125°C, 5V = 5 MHz
    // so we're overclocking here, but it works for me™
    .clock_speed_hz = 20*1000*1000,
    .mode = 0,
    .pre_cb  = vfd_spi_strobe_low,
    .post_cb = vfd_spi_strobe_high,
    .command_bits = 0,
    .address_bits = 0,
    .dummy_bits = 0,
    .duty_cycle_pos = 128,
    .spics_io_num = -1,
    .queue_size = 4,
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
#define vfd_grids_n (sizeof(vfd_grids)/sizeof(uint16_t))

// periodically called function which does time-multiplexing of the
// individual digits on the display from contents of vfd->buf
void IRAM_ATTR vfd_digitmux_task(vfd_handle_t *vfd) {

  // setup local variables and acquire bus
  ESP_ERROR_CHECK(spi_device_acquire_bus(vfd->spi, portMAX_DELAY));
  int p;
  uint16_t data;

  // loop unblocked from timer isr periodically
  while (true) {
    xSemaphoreTake(vfd->mux, portMAX_DELAY);
    p = vfd->pos;
    data = (vfd->buf[p] & SEGMENTMASK) | vfd_grids[p];
    vfd_spi_data(vfd, data);
    vfd->pos = (p + 1) % vfd_grids_n;
  }

}

// interrupt service routine that periodically unblocks vfd_digitmux_task
void IRAM_ATTR vfd_digitmux_isr(vfd_handle_t *vfd) {
  // rearm timer
  (vfd->timer.idx) ? (TIMERG0.int_clr_timers.t1 = 1) : (TIMERG0.int_clr_timers.t0 = 1);
  TIMERG0.hw_timer[vfd->timer.idx].config.alarm_en = TIMER_ALARM_EN;
  // unblock and wake task
  BaseType_t HigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(vfd->mux, &HigherPriorityTaskWoken);
}

// vfd_mux_init configures a hardware timer and registers an isr for
// digit multiplexing in the background. use period=0.002 for 
// approximately 60 Hz overall refresh rate.
#define HVMUX_DIVIDER 80
void vfd_mux_init(vfd_handle_t *vfd, timer_group_t group, timer_idx_t idx, double period) {

  // only timer group 0 / TIMERG0 is supported for now
  if (group != 0) {
    ESP_LOGW(vfd->tag, "only timer group 0 supported for now");
    group = 0;
  }

  // initialise multiplexing timer
  vfd->timer = (vfd_timer_t){ group, idx };
  timer_config_t timercfg = {
    .divider = HVMUX_DIVIDER,
    .counter_en = false,
    .counter_dir = TIMER_COUNT_UP,
    .auto_reload = TIMER_AUTORELOAD_EN,
    .alarm_en = true,
    .intr_type = TIMER_INTR_LEVEL,
  };
  timer_init(group, idx, &timercfg);
  timer_set_counter_value(group, idx, 0);
  timer_set_alarm_value(group, idx, period * (TIMER_BASE_CLK / HVMUX_DIVIDER));

  // create task and register isr
  vfd->mux = xSemaphoreCreateBinary();
  xTaskCreate((void(*)(void*))vfd_digitmux_task, "digitmux", 2048, vfd, 20, NULL);
  timer_enable_intr(group, idx);
  timer_isr_register(group, idx, (void(*)(void*))vfd_digitmux_isr, vfd, 0, NULL);
  timer_start(group, idx);

}

// write text into the buffer from which digit multiplexing is performed.
// text must point to a char array with $GRIDS elements
void vfd_text(vfd_handle_t *vfd, char *text) {
  int i = 0;
  while ((i < GRIDS) && (text[i] != 0)) {
    vfd->buf[i] = segment_lookup(text[i]);
    i++;
  };
}

// ------------------------ convenience ------------------------

// convenience function to init display with defaults on chronovfd
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
  vfd_mux_init(vfd, TIMER_GROUP_0, TIMER_0, 0.002);

  return vfd;

}
