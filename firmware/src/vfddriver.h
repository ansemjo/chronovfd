#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <driver/timer.h>
#include "segments.h"

// ------------------------ hv5812 spi driver ------------------------

// default interface to the hv5812 on chronovfd
#define VFD_SPI_HOST    HSPI_HOST
#define VFD_PIN_ENABLE  GPIO_NUM_19
#define VFD_PIN_CLOCK   GPIO_NUM_25
#define VFD_PIN_DATA    GPIO_NUM_26
#define VFD_PIN_STROBE  GPIO_NUM_27
#define VFD_PIN_BLANK   GPIO_NUM_18
#define VFD_PIN_FILSHDN GPIO_NUM_17
#define VFD_PIN_HVSHDN  GPIO_NUM_16

// vfd_pin_t holds relevant gpio numbers
// connected to the hv5812 display driver
typedef struct vfd_pin_t {
  gpio_num_t enable;
  gpio_num_t clock;
  gpio_num_t data;
  gpio_num_t strobe;
  gpio_num_t blank;
  gpio_num_t fil_shdn;
  gpio_num_t hv_shdn;
} vfd_pin_t;

// vfd_handle_t holds a handle to the instantiated
// spi device and the relevant gpio pins
typedef struct vfd_handle_t {
  vfd_pin_t pin;
  spi_device_handle_t spidev;
  uint16_t buf[5]; // display contents
} vfd_handle_t;

void vfd_init_spi(vfd_pin_t *pin);
void IRAM_ATTR vfd_transmit(uint16_t data);
void vfd_text(const char text[GRIDS]);
void vfd_raw(uint16_t raw[GRIDS]);

// ------------------------ digitmux timer ------------------------

#define VFD_MUX_DIVIDER       80
#define VFD_MUX_SECOND        (TIMER_BASE_CLK / VFD_MUX_DIVIDER)
#define VFD_MUX_TIMERG_DEV    TIMERG0
#define VFD_MUX_TIMER_GROUP   0
#define VFD_MUX_TIMER_IDX     0
#define VFD_TIMER             VFD_MUX_TIMER_GROUP, VFD_MUX_TIMER_IDX

void vfd_init_mux(double period, TaskHandle_t *task);
void vfd_set_mux_period(double period);
