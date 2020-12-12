#pragma once

#include "esp_event.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "nvs_flash.h"

#include "protocol_examples_common.h"

#include "app_led.hpp"
#include "app_recognizer.hpp"
#include "app_remote.hpp"
#include "app_rmt_light.hpp"
#include "app_rmt_ac.hpp"

constexpr auto GPIO_LED_RED = GPIO_NUM_21;
constexpr auto GPIO_LED_WHITE = GPIO_NUM_22;

constexpr auto GPIO_BUTTON = GPIO_NUM_15;
constexpr auto GPIO_BOOT = GPIO_NUM_0;

constexpr auto GPIO_IR_TX = GPIO_NUM_2;
constexpr auto GPIO_IR_RX = GPIO_NUM_19;
