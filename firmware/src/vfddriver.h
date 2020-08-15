#ifndef _VFDDRIVER_H_
#define _VFDDRIVER_H_

// vfddriver_pins_t holds relevant gpio numbers
// connected to the hv5812 display driver
typedef struct vfd_pins_t {
  gpio_num_t enable;
  gpio_num_t clock;
  gpio_num_t data;
  gpio_num_t strobe;
  gpio_num_t blank;
  gpio_num_t fil_shdn;
  gpio_num_t hv_shdn;
} vfd_pins_t;

// vfddriver_handle_t holds a handle to the instantiated
// spi device and the relevant gpio pins
typedef struct vfd_handle_t {
  spi_device_handle_t spidev;
  vfd_pins_t pin;
} vfd_handle_t;

vfd_handle_t vfd_init(spi_host_device_t host, vfd_pins_t pin, char *tag);
void vfd_data(vfd_handle_t *vfd, uint16_t data);

#endif // _VFDDRIVER_H_
