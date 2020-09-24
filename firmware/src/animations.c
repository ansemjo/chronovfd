#include <stdint.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "segments.h"
#include "vfddriver.h"

// loading animation task with two vertices rotating on the
// outermost segments around the display. infinite loop, kill via handle!
void animation_spinner_task(void *arg) {

  // delay between animation frames
  TickType_t dl = 80 * portTICK_RATE_MS;
  TickType_t last = xTaskGetTickCount();

  // animate intro
  uint16_t intro[][GRIDS] = {
    {     0,     0, 0,     0,     0 },
    {    Aa,     0, 0,     0,    Ad },
  };
  for (int i = 0; i < (sizeof(intro)/sizeof(intro[0])); i++) {
    vfd_raw(intro[i]);
    vTaskDelayUntil(&last, dl);
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
    vTaskDelayUntil(&last, dl);
  }

}

void animation_spinner(TaskHandle_t *task) {
  xTaskCreate(animation_spinner_task, "anim:spinner", 1024, NULL, 2, task);
}


void animation_textrunner_task(void *arg) {

  // delay between animation frames
  TickType_t dl = 220 * portTICK_RATE_MS;
  TickType_t last = xTaskGetTickCount();

  // cast arg pointer to char string
  const char *text = arg;
  const int len = strlen(text);

  uint16_t buf[GRIDS];
  for (;;) {
  for (int i = 0; i < len; i++){ //i = (i + 1) % len) {
    buf[0] = segment_lookup(text[i]);
    buf[1] = segment_lookup(text[(i + 1) % len]);
    buf[2] = 0; // dots
    buf[3] = segment_lookup(text[(i + 2) % len]);
    buf[4] = segment_lookup(text[(i + 3) % len]);
    vfd_raw(buf);
    vTaskDelayUntil(&last, dl);
  };
  vfd_text("  :3 ");
  vTaskDelayUntil(&last, 700 * portTICK_RATE_MS);

  }

}

void animation_textrunner(TaskHandle_t *task, char *text) {
  xTaskCreate(animation_textrunner_task, "anim:textrunner", 2048, text, 2, task);
}


void animation_fader_task(void *arg) {

  // delay between animation frames
  TickType_t dl = 20 * portTICK_RATE_MS;
  TickType_t last = xTaskGetTickCount();

  // empty intro frame
  vfd_raw((uint16_t[GRIDS]){ 0, 0, 0, 0, 0 });
  vTaskDelayUntil(&last, dl);
  
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
    vTaskDelayUntil(&last, dl);
  }

}

void animation_fader(TaskHandle_t *task) {
  xTaskCreate(animation_fader_task, "anim:fader", 1024, NULL, 2, task);
}

void animation_textfader_task(void *arg) {

  // delay between animation frames
  TickType_t dl = 20 * portTICK_RATE_MS;
  TickType_t last = xTaskGetTickCount();

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
    vTaskDelayUntil(&last, dl);
  }

}

void animation_textfader(TaskHandle_t *task, char text[4]) {
  xTaskCreate(animation_textfader_task, "anim:textfader", 1024, text, 2, task);
}
