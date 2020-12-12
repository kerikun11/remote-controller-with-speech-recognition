#pragma once

#include <array>

#include "esp_log.h"

#include "driver/gpio.h"

namespace app {

class led {
public:
  led(const std::array<gpio_num_t, 2> &pins) : pins(pins) {
    for (const auto pin : pins) {
      gpio_set_level(pin, 0);
      gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    }
  }
  void write(int r, int w) {
    gpio_set_level(pins[0], r);
    gpio_set_level(pins[1], w);
  }

private:
  const std::array<gpio_num_t, 2> pins;
};

} // namespace app
