#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void animation_spinner(TaskHandle_t *task);
void animation_fader(TaskHandle_t *task);
void animation_textfader(TaskHandle_t *task, const char text[4]);