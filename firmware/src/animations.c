#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "segments.h"
#include "vfddriver.h"

// loading animation task with two vertices rotating on the
// outermost segments around the display. infinite loop, kill via handle!
void animation_spinner_task(void *arg) {

  // delay between animation frames
  TickType_t dl = 80 * portTICK_RATE_MS;

  // animate intro
  uint16_t intro[][GRIDS] = {
    {     0,     0, 0,     0,     0 },
    {    Aa,     0, 0,     0,    Ad },
  };
  for (int i = 0; i < (sizeof(intro)/sizeof(intro[0])); i++) {
    vfd_raw(intro[i]);
    vTaskDelay(dl);
  }
  
  // infinite loop of main animation
  uint16_t animation[][GRIDS] = {
    {    Aa,    Aa, 0,    Ad,    Ad },
    {     0, Aa|Ad, 0, Aa|Ad,     0 },
    {    Ad,    Ad, 0,    Aa,    Aa },
    { Ad|Ae,     0, 0,     0, Aa|Ab },
    { Ae|Af,     0, 0,     0, Ab|Ac },
    { Aa|Af,     0, 0,     0, Ac|Ad },
  };
  for (int i = 0;; i = (i + 1) % (sizeof(animation)/sizeof(animation[0]))) {
    vfd_raw(animation[i]);
    vTaskDelay(dl);
  }

}

void animation_spinner(TaskHandle_t *task) {
  xTaskCreate(animation_spinner_task, "anim:spinner", 1024, NULL, 2, task);
}