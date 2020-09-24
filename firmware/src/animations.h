// Copyright (c) 2020 Anton Semjonov
// Licensed under the MIT License

#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void animation_spinner(TaskHandle_t *task);
void animation_fader(TaskHandle_t *task);
void animation_textfader(TaskHandle_t *task, const char text[4]);
void animation_textrunner(TaskHandle_t *task, const char *text);