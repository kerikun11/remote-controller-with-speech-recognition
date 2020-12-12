/**
 * @file app_main.cpp
 * @author Ryotaro Onuki (kerikun11+github@gmail.com)
 * @brief main application
 * @date 2020-12-06
 * @copyright Copyright (c) 2020 Ryotaro Onuki
 */
#include "app_main.hpp"

static constexpr const char *TAG = "app_main";

extern "C" void app_main() {
  ESP_LOGI(TAG, "Hello, this is ESP32.");

  // ESP_ERROR_CHECK(nvs_flash_init());
  // ESP_ERROR_CHECK(esp_netif_init());
  // ESP_ERROR_CHECK(esp_event_loop_create_default());
  // ESP_ERROR_CHECK(example_connect());

  app::led led({GPIO_LED_RED, GPIO_LED_WHITE});
  app::remote::tx remote_tx(GPIO_IR_TX);
  app::remote::rx remote_rx(GPIO_IR_RX);
  app::recognizer recognizer;
  app::remote::rmt_raw_data_t rmt_data;

  while (1) {
    app::recognizer::Event event;
    recognizer.receive(event);
    switch (event.id) {
    case app::recognizer::EventId::WOKE_UP:
      led.write(0, 1);
      break;
    case app::recognizer::EventId::GOT_COMMAND:
      switch (event.command_id) {
      // case 0:
      case 18:
        led.write(0, 0);
        remote_tx.transmit(
            app::remote::signal::get(app::remote::signal::LIGHT_ON));
        break;
      // case 1:
      case 19:
        led.write(0, 0);
        remote_tx.transmit(
            app::remote::signal::get(app::remote::signal::LIGHT_OFF));
        break;
      case 2:
        led.write(1, 1);
        rmt_data = remote_rx.receive();
        break;
      case 3:
        led.write(0, 0);
        rmt_data = remote_rx.receive();
        break;
      default:
        led.write(1, 0);
        break;
      }
      break;
    }
  }
}
