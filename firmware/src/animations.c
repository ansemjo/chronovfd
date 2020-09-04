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



void animation_fader_task(void *arg) {

  // delay between animation frames
  TickType_t dl = 20 * portTICK_RATE_MS;

  // empty intro frame
  vfd_raw((uint16_t[GRIDS]){ 0, 0, 0, 0, 0 });
  vTaskDelay(dl);
  
  // helper variables
  uint16_t left = Aa|Ad|Ae|Af;
  uint16_t right = Ab|Ac|Ag;
  uint16_t g1, g2, g4, g5;

  // infinite loop of main animation

  for (int i = 0; i < 17; i = (i + 1) % 17) {
    g1 = g2 = g4 = g5 = 0;
    switch (i) {
      default:
        __attribute__((fallthrough));
      case 8:
        g5 |= right; __attribute__((fallthrough));
      case 7:
        g5 |= left; __attribute__((fallthrough));
      case 6:
        g4 |= right; __attribute__((fallthrough));
      case 5:
        g4 |= left; __attribute__((fallthrough));
      case 4:
        g2 |= right; __attribute__((fallthrough));
      case 3:
        g2 |= left; __attribute__((fallthrough));
      case 2:
        g1 |= right; __attribute__((fallthrough));
      case 1:
        g1 |= left; __attribute__((fallthrough));
      case 0:
        break;
    }
    switch (i) {
      case 16:
        g5 ^= right; __attribute__((fallthrough));
      case 15:
        g5 ^= left; __attribute__((fallthrough));
      case 14:
        g4 ^= right; __attribute__((fallthrough));
      case 13:
        g4 ^= left; __attribute__((fallthrough));
      case 12:
        g2 ^= right; __attribute__((fallthrough));
      case 11:
        g2 ^= left; __attribute__((fallthrough));
      case 10:
        g1 ^= right; __attribute__((fallthrough));
      case 9:
        g1 ^= left; __attribute__((fallthrough));
      default:
        break;
    }
    vfd_raw((uint16_t[GRIDS]){ g1, g2, 0, g4, g5 });
    vTaskDelay(dl);
  }

}

void animation_fader(TaskHandle_t *task) {
  xTaskCreate(animation_fader_task, "anim:fader", 1024, NULL, 2, task);
}

void animation_textfader_task(void *arg) {

  // delay between animation frames
  TickType_t dl = 20 * portTICK_RATE_MS;

  // helper variables
  uint16_t left = Ad|Ae|Af;
  uint16_t right = Aa|Ab|Ac|Ag;
  uint16_t g1, g2, g4, g5;

  // infinite loop of main animation

  for (int i = 0; i < 17; i = (i + 1) % 17) {
    
    g1 = segment_lookup(((char*)arg)[0]);
    g2 = segment_lookup(((char*)arg)[1]);
    g4 = segment_lookup(((char*)arg)[2]);
    g5 = segment_lookup(((char*)arg)[3]);

    switch (i) {
      case 0:
        g1 &= right; __attribute__((fallthrough));
      case 1:
        g1 &= left; __attribute__((fallthrough));
      case 2:
        g2 &= right; __attribute__((fallthrough));
      case 3:
        g2 &= left; __attribute__((fallthrough));
      case 4:
        g4 &= right; __attribute__((fallthrough));
      case 5:
        g4 &= left; __attribute__((fallthrough));
      case 6:
        g5 &= right; __attribute__((fallthrough));
      case 7:
        g5 &= left; __attribute__((fallthrough));
      default:
        break;
    }
    switch (i) {
      case 16:
        g5 &= left; __attribute__((fallthrough));
      case 15:
        g5 &= right; __attribute__((fallthrough));
      case 14:
        g4 &= left; __attribute__((fallthrough));
      case 13:
        g4 &= right; __attribute__((fallthrough));
      case 12:
        g2 &= left; __attribute__((fallthrough));
      case 11:
        g2 &= right; __attribute__((fallthrough));
      case 10:
        g1 &= left; __attribute__((fallthrough));
      case 9:
        g1 &= right; __attribute__((fallthrough));
      default:
        break;
    }
    vfd_raw((uint16_t[GRIDS]){ g1, g2, 0, g4, g5 });
    vTaskDelay(dl);
  }

}

void animation_textfader(TaskHandle_t *task, const char text[4]) {
  xTaskCreate(animation_textfader_task, "anim:textfader", 1024, text, 2, task);
}
