// Copyright (c) 2020 Anton Semjonov
// Licensed under the MIT License

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

// ------------------------ hv5812 spi driver ------------------------

#define TAG "chronovfd"

static vfd_handle_t vfd;

// set strobe low before spi transaction
void IRAM_ATTR vfd_strobe_low(spi_transaction_t *t) {
  gpio_set_level(vfd.pin.strobe, 0);
}

// set strobe high after spi transaction to latch contents
void IRAM_ATTR vfd_strobe_high(spi_transaction_t *t) {
  gpio_set_level(vfd.pin.strobe, 1);
}

// transmit data to a hv5812 vfd driver
void IRAM_ATTR vfd_transmit(uint16_t data) {
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));

  // prepare data to be sent, 20 bits fully define all output pins
  uint32_t buf = SPI_SWAP_DATA_TX(data & (GRIDMASK|SEGMENTMASK), 20);
  t.tx_buffer = &buf;
  t.length = 20;

  // transmit and wait for transaction to finish
  ESP_ERROR_CHECK(spi_device_polling_transmit(vfd.spidev, &t));
}

// shorthand to prepare gpio outputs
#define gpio_init_output(pin, level) \
  ESP_ERROR_CHECK(gpio_set_direction(pin, GPIO_MODE_OUTPUT)); \
  ESP_ERROR_CHECK(gpio_set_level(pin, level))

// initialize spi interface to hv5812 display driver with auxiliary pins
void vfd_init_spi(vfd_pin_t *pin) {
  vfd.pin = *pin;

  // configure the spi bus
  spi_bus_config_t buscfg = {
    .sclk_io_num   = vfd.pin.clock,
    .mosi_io_num   = vfd.pin.data,
    .miso_io_num   = -1,
    .quadhd_io_num = -1,
    .quadwp_io_num = -1,
  };
  ESP_ERROR_CHECK(spi_bus_initialize(VFD_SPI_HOST, &buscfg, 0));
  ESP_LOGI(TAG, "hspi bus configured with { clock: %d, data: %d }",
    vfd.pin.clock, vfd.pin.data);
  
  // add the device on the bus
  spi_device_interface_config_t devcfg = {
    // HV5812 maximum rated frequency @ 125°C, 5V is 5 MHz
    // we're overclocking here, but it works for me™
    .clock_speed_hz = 20*1000*1000,
    .mode = 0,
    .pre_cb  = vfd_strobe_low,
    .post_cb = vfd_strobe_high,
    .command_bits = 0,
    .address_bits = 0,
    .dummy_bits = 0,
    .duty_cycle_pos = 128,
    .spics_io_num = -1,
    .queue_size = 4,
  };
  ESP_ERROR_CHECK(spi_bus_add_device(VFD_SPI_HOST, &devcfg, &vfd.spidev));
  ESP_LOGI(TAG, "device added to bus with { strobe: %d }", vfd.pin.strobe);

  // configure auxiliary gpios for display driver
  gpio_init_output( vfd.pin.strobe,   0 ); // don't latch immediately
  gpio_init_output( vfd.pin.blank,    0 ); // don't blank outputs
  gpio_init_output( vfd.pin.fil_shdn, 0 ); // filament supply on
  gpio_init_output( vfd.pin.hv_shdn,  1 ); // high-voltage supply on
  gpio_init_output( vfd.pin.enable,   0 ); // enable level shifter

  // set all hv pins to off once
  vfd_transmit(0);

}

// write text into the buffer from which digit multiplexing is performed.
// text must point to a char array with $GRIDS elements
void vfd_text(const char text[GRIDS]) {
  bool end = false;
  for (int i = 0; i < GRIDS; i++) {
    if (text[i] == 0) end = true;
    if (end)
      vfd.buf[i] = 0;
    else
      vfd.buf[i] = segment_lookup(text[i]);
  };
}

// copy a raw display buffer, when segment mapping has already been performed
void vfd_raw(uint16_t raw[GRIDS]) {
  memcpy(vfd.buf, raw, sizeof(uint16_t) * GRIDS);
}


// ------------------------ digitmux timer ------------------------

static IRAM_ATTR TaskHandle_t digitmux;

// periodically called function which does time-multiplexing of the
// individual digits on the display from contents of vfd->buf
void IRAM_ATTR vfd_digitmux_task(void *arg) {
  static int pos;
  uint16_t data;

  // acquire bus exclusively to speed up transactions
  ESP_ERROR_CHECK(spi_device_acquire_bus(vfd.spidev, portMAX_DELAY));

  // store own task handle for isr
  digitmux = xTaskGetCurrentTaskHandle();

  for (;;) {
    // unblocked from isr periodically
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    data = (vfd.buf[pos] & SEGMENTMASK) | grids[pos];
    vfd_transmit(data);
    pos = (pos + 1) % GRIDS;
  }

}

// interrupt service routine that periodically unblocks vfd_digitmux_task
void IRAM_ATTR vfd_digitmux_isr(void *arg) {
  // rearm timer
#if VFD_MUX_TIMER_IDX == 0
  VFD_MUX_TIMERG_DEV.int_clr_timers.t0 = 1;
#else
  VFD_MUX_TIMERG_DEV.int_clr_timers.t1 = 1;
#endif
  VFD_MUX_TIMERG_DEV.hw_timer[VFD_MUX_TIMER_IDX].config.alarm_en = TIMER_ALARM_EN;
  // unblock and wake task
  if (digitmux) {
    BaseType_t HigherPriorityTaskWoken = pdTRUE;
    vTaskNotifyGiveFromISR(digitmux, &HigherPriorityTaskWoken);
    portYIELD_FROM_ISR();
  }
}


// vfd_mux_init configures a hardware timer and registers an isr for
// digit multiplexing in the background. use period=0.002 for 
// approximately 60 Hz overall refresh rate.
void vfd_init_mux(double period, TaskHandle_t *task) {

  // initialise multiplexing timer
  timer_config_t timercfg = {
    .divider = VFD_MUX_DIVIDER,
    .counter_en = false,
    .counter_dir = TIMER_COUNT_UP,
    .auto_reload = TIMER_AUTORELOAD_EN,
    .alarm_en = true,
    .intr_type = TIMER_INTR_LEVEL,
  };
  ESP_ERROR_CHECK(timer_init(VFD_TIMER, &timercfg));
  
  // reset counter and set alarm value for given period
  ESP_ERROR_CHECK(timer_set_counter_value(VFD_TIMER, 0));
  ESP_ERROR_CHECK(timer_set_alarm_value(VFD_TIMER, period * VFD_MUX_SECOND));
  ESP_LOGI(TAG, "digitmux timer prepared");

  // register isr and start timer
  ESP_ERROR_CHECK(timer_enable_intr(VFD_TIMER));
  ESP_ERROR_CHECK(timer_isr_register(VFD_TIMER, vfd_digitmux_isr, NULL, 0, NULL));
  ESP_ERROR_CHECK(timer_start(VFD_TIMER));

  // explicitly zero the display buffer
  bzero(vfd.buf, sizeof(vfd.buf));

  // create mux task and return its handle
  xTaskCreate(vfd_digitmux_task, "digitmux", 2048, NULL, configMAX_PRIORITIES - 1, task);
  // default event loop is pinned on core 0, so if you see flickering maybe pin to core 1 ..
  // xTaskCreatePinnedToCore(vfd_digitmux_task, "digitmux", 2048, NULL, configMAX_PRIORITIES - 1, task, 1);
  ESP_LOGI(TAG, "digitmux isr registered & task created");

}

void vfd_set_mux_period(double period) {
  ESP_ERROR_CHECK(timer_pause(VFD_TIMER));
  ESP_ERROR_CHECK(timer_set_alarm_value(VFD_TIMER, period * VFD_MUX_SECOND));
  uint64_t cnt;
  ESP_ERROR_CHECK(timer_get_counter_value(VFD_TIMER, &cnt));
  if (!(cnt < (period * VFD_MUX_SECOND))) {
    ESP_ERROR_CHECK(timer_set_counter_value(VFD_TIMER, (period * VFD_MUX_SECOND) - 1));
  }
  ESP_ERROR_CHECK(timer_start(VFD_TIMER));
  ESP_LOGV(TAG, "digitmux period changed to %0.3fs", period);
}
