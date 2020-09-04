#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <driver/timer.h>

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
  spi_host_device_t host;
  gpio_num_t enable;
  gpio_num_t clock;
  gpio_num_t data;
  gpio_num_t strobe;
  gpio_num_t blank;
  gpio_num_t fil_shdn;
  gpio_num_t hv_shdn;
} vfd_pin_t;

// vfd_timer_t holds configuration options to the
// hardware timer used for digit multiplexing
typedef struct vfd_timer_t {
  timer_group_t group;
  timer_idx_t idx;
} vfd_timer_t;

// vfd_handle_t holds a handle to the instantiated
// spi device and the relevant gpio pins
typedef struct vfd_handle_t {
  char *tag; // for logging
  vfd_pin_t pin; // pin definitions to hv5812
  spi_device_handle_t spi; // spi handle
  vfd_timer_t timer; // timer configuration
  xSemaphoreHandle mux; // digitmux lock
  uint16_t buf[5]; // display contents
  int pos; // current digit
} vfd_handle_t;

vfd_handle_t* vfd_init(vfd_pin_t pin, char *tag);
void vfd_spi_data(vfd_handle_t *vfd, uint16_t data);
void vfd_mux_init(vfd_handle_t *vfd, timer_group_t group, timer_idx_t idx, double period);
void vfd_text(vfd_handle_t *vfd, char *text);
void vfd_raw(vfd_handle_t *vfd, uint16_t raw[]);

vfd_handle_t* chronovfd_init();
