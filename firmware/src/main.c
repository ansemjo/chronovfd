#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_types.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"

#include "segments.h"

// define interface to the hv5812
#define HV_SPI  HSPI_HOST
#define DMA_CHAN        2
#define HV_PIN_ENABLE  GPIO_NUM_19
#define HV_PIN_CLOCK   GPIO_NUM_25
#define HV_PIN_DATA    GPIO_NUM_26
#define HV_PIN_STROBE  GPIO_NUM_27
#define HV_PIN_BLANK   GPIO_NUM_18
#define HV_PIN_FILSHDN GPIO_NUM_17
#define HV_PIN_HVSHDN  GPIO_NUM_16

#define HVMUX_GROUP   0
#define HVMUX_ISRG    TIMERG0
#define HVMUX_IDX     0
#define HVMUX_DIVIDER 80

// toggle the strobe line to display transmitted digit
void hv_spi_pre_strobe(spi_transaction_t *t) {
  gpio_set_level(HV_PIN_STROBE, 0);
}
void hv_spi_post_strobe(spi_transaction_t *t) {
  gpio_set_level(HV_PIN_STROBE, 1);
}

// send data
void hv_data(spi_device_handle_t hv, uint16_t data) {
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  t.flags = SPI_TRANS_USE_TXDATA;
  t.length = 16;
  t.tx_data[0] = (data >> 8) & 0xFF;
  t.tx_data[1] =  data       & 0xFF;
  // t.tx_data[0] = 0x00;
  // t.tx_data[1] = 0xff ^ (G2|G3);
  hv_spi_pre_strobe(&t);
  esp_err_t ret = spi_device_transmit(hv, &t);
  hv_spi_post_strobe(&t);
  ESP_ERROR_CHECK(ret);
}

// initialize hv to display all segments
void hv_init(spi_device_handle_t hv) {

  // configure aux gpios for display
  gpio_set_direction(HV_PIN_ENABLE,  GPIO_MODE_OUTPUT);
  gpio_set_direction(HV_PIN_STROBE,  GPIO_MODE_OUTPUT);
  gpio_set_direction(HV_PIN_BLANK,   GPIO_MODE_OUTPUT);
  gpio_set_direction(HV_PIN_FILSHDN, GPIO_MODE_OUTPUT);
  gpio_set_direction(HV_PIN_HVSHDN,  GPIO_MODE_OUTPUT);
  gpio_set_level(HV_PIN_BLANK, 0);
  gpio_set_level(HV_PIN_FILSHDN, 0);
  gpio_set_level(HV_PIN_HVSHDN, 1);
  gpio_set_level(HV_PIN_ENABLE, 0);
  // vTaskDelay(100 / portTICK_RATE_MS);
  // gpio_set_level(HV_PIN_FILSHDN, 1);

  // const uint16_t data = Aa | G1|G2|G3|G4|Gd;
  uint16_t data = lookup('H') | G1|G2|G3|G4;
  ESP_LOGI("segments", "lookup: %x", lookup('H'));
  ESP_LOGI("segments", "direct: %x", Ab|Ac|Ae|Af|Ag);
  hv_data(hv, data);

}

volatile bool MUXON = true;
void hv_mux(void *null) {
  HVMUX_ISRG.int_clr_timers.t0 = 1;
  HVMUX_ISRG.hw_timer[HVMUX_IDX].config.alarm_en = TIMER_ALARM_EN;
  MUXON = !MUXON;
  gpio_set_level(HV_PIN_HVSHDN, MUXON);
}

#define LED 5
void led_init() {
  gpio_set_direction(LED, GPIO_MODE_OUTPUT);
  gpio_set_level(LED, 1);
}

void app_main() {

  esp_err_t ret;
    
  // init default nonvolatile storage
  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // init spi line to hv shifter
  spi_device_handle_t hv;
  spi_bus_config_t hvbuscfg = {
    .sclk_io_num = HV_PIN_CLOCK,
    .mosi_io_num = HV_PIN_DATA,
    .miso_io_num = -1,
    .quadhd_io_num = -1,
    .quadwp_io_num = -1,
  };
  spi_device_interface_config_t hvdevcfg = {
    .clock_speed_hz = 5*1000*1000, // maximum is 5MHz @ 125°C, 5V
    .mode = 0,
    // .pre_cb  = hv_spi_pre_strobe,
    // .post_cb = hv_spi_post_strobe,
    .command_bits = 0,
    .address_bits = 0,
    .dummy_bits = 0,
    .duty_cycle_pos = 128,
    .spics_io_num = -1,
    .queue_size=1,
  };
  ESP_ERROR_CHECK(spi_bus_initialize(HV_SPI, &hvbuscfg, 1));
  ESP_LOGI("hv5812", "bus configured");
  ESP_ERROR_CHECK(spi_bus_add_device(HV_SPI, &hvdevcfg, &hv));
  ESP_LOGI("hv5812", "device configured");
  hv_init(hv);

  // initialise multiplexing timer
  timer_config_t hvmuxcfg = {
    .divider = HVMUX_DIVIDER,
    .counter_en = false,
    .counter_dir = TIMER_COUNT_UP,
    .auto_reload = TIMER_AUTORELOAD_EN,
    .alarm_en = true,
    .intr_type = TIMER_INTR_LEVEL,
  };
  timer_init(HVMUX_GROUP, HVMUX_IDX, &hvmuxcfg);
  timer_set_counter_value(HVMUX_GROUP, HVMUX_IDX, 0);
  timer_set_alarm_value(HVMUX_GROUP, HVMUX_IDX, 1 * (TIMER_BASE_CLK / HVMUX_DIVIDER));
  timer_enable_intr(HVMUX_GROUP, HVMUX_IDX);
  timer_isr_register(HVMUX_GROUP, HVMUX_IDX, &hv_mux, NULL, 0, NULL);
  timer_start(HVMUX_GROUP, HVMUX_IDX);

  led_init();
  ESP_LOGI("LED", "turned on");


  //wifi_init_softap();

}

