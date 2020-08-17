#ifndef _VFDDRIVER_H_
#define _VFDDRIVER_H_

// default interface to the hv5812 on chronovfd
#define VFD_SPI_HOST    HSPI_HOST
#define VFD_PIN_ENABLE  GPIO_NUM_19
#define VFD_PIN_CLOCK   GPIO_NUM_25
#define VFD_PIN_DATA    GPIO_NUM_26
#define VFD_PIN_STROBE  GPIO_NUM_27
#define VFD_PIN_BLANK   GPIO_NUM_18
#define VFD_PIN_FILSHDN GPIO_NUM_17
#define VFD_PIN_HVSHDN  GPIO_NUM_16

// default digit multiplex timer settings
#define HVMUX_GROUP   0
#define HVMUX_ISRG    TIMERG0
#define HVMUX_IDX     0
#define HVMUX_DIVIDER 16

// vfddriver_pins_t holds relevant gpio numbers
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

// vfddriver_handle_t holds a handle to the instantiated
// spi device and the relevant gpio pins
typedef struct vfd_handle_t {
  spi_device_handle_t spi;
  vfd_pin_t pin;
} vfd_handle_t;

vfd_handle_t vfd_init(vfd_pin_t *pin, char *tag);
void vfd_data(vfd_handle_t *vfd, uint16_t data);

uint16_t vfd_rawbuf[5];
void vfd_mux_init();

#endif // _VFDDRIVER_H_
