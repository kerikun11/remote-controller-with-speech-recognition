#pragma once

#include "driver/rmt.h"
#include "esp_log.h"

#include <iostream>
#include <memory>
#include <vector>

namespace app {
namespace remote {

using rmt_raw_data_t = std::vector<rmt_item32_t>;
std::ostream &operator<<(std::ostream &os, const rmt_raw_data_t &data) {
  os << "{";
  for (const auto &v : data) {
    // os << "{" << v.duration0 << "," << v.duration1 << "},";
    os << v.val << ",";
  }
  os << "}";
  return os;
}

class tx {
public:
  tx(gpio_num_t gpio_num, rmt_channel_t rmt_channel = RMT_CHANNEL_0)
      : rmt_channel(rmt_channel),
        queue(xQueueCreate(10, sizeof(rmt_raw_data_t *))) {
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(gpio_num, rmt_channel);
    config.tx_config.carrier_en = true;
    config.clk_div = 80 * 2;
    ESP_ERROR_CHECK(rmt_config(&config));
    xTaskCreate(
        [](void *_this) { reinterpret_cast<decltype(this)>(_this)->task(); },
        TAG, 4 * configMINIMAL_STACK_SIZE, this, 0, NULL);
  }
  BaseType_t transmit(const rmt_raw_data_t &rmt_data,
                      TickType_t xTicksToWait = portMAX_DELAY) {
    auto p = new rmt_raw_data_t(rmt_data);
    return xQueueSendToBack(queue, &p, xTicksToWait);
  }

private:
  static constexpr char const *TAG = "remote_tx";
  rmt_channel_t rmt_channel;
  QueueHandle_t queue;

  void task() {
    // install RMT driver
    ESP_ERROR_CHECK(rmt_driver_install(rmt_channel, 0, 0));
    while (1) {
      rmt_raw_data_t *p;
      xQueueReceive(queue, &p, portMAX_DELAY);
      if (!p)
        continue;
      const std::unique_ptr<rmt_raw_data_t> p_rmt_data(p);
      const rmt_item32_t *items = p_rmt_data->data();
      const int length = p_rmt_data->size();
      ESP_LOGI(TAG, "rmt_tx_start");
      rmt_write_items(rmt_channel, items, length, true); //< must be true
      std::cout << "tx: " << *p_rmt_data << std::endl;
    }
    ESP_ERROR_CHECK(rmt_driver_uninstall(rmt_channel));
    vTaskDelete(NULL);
  }
};

class rx {
public:
  rx(gpio_num_t gpio_num, rmt_channel_t rmt_channel = RMT_CHANNEL_1)
      : rmt_channel(rmt_channel),
        queue(xQueueCreate(1, sizeof(rmt_raw_data_t *))) {
    rmt_config_t config = RMT_DEFAULT_CONFIG_RX(gpio_num, rmt_channel);
    config.mem_block_num = 6;
    config.clk_div = 80 * 2;
    config.rx_config.idle_threshold = 32767;
    ESP_ERROR_CHECK(rmt_config(&config));
    xTaskCreate(
        [](void *_this) { reinterpret_cast<decltype(this)>(_this)->task(); },
        TAG, 4 * configMINIMAL_STACK_SIZE, this, 0, NULL);
  }
  rmt_raw_data_t receive(TickType_t xTicksToWait = portMAX_DELAY) {
    rmt_raw_data_t *p;
    if (xQueueReceive(queue, &p, xTicksToWait) == pdFALSE)
      return rmt_raw_data_t();
    return *std::unique_ptr<rmt_raw_data_t>(p); // release allocated memory
  }

private:
  static constexpr char const *TAG = "remote_rx";
  rmt_channel_t rmt_channel;
  QueueHandle_t queue;

  void task() {
    // install RMT driver
    ESP_ERROR_CHECK(rmt_driver_install(rmt_channel, 4 * 1024, 0));
    // get RMT RX ringbuffer
    RingbufHandle_t rb = NULL;
    ESP_ERROR_CHECK(rmt_get_ringbuf_handle(rmt_channel, &rb));

    while (rb) {
      // Start receive
      ESP_LOGI(TAG, "rmt_rx_start");
      ESP_ERROR_CHECK(rmt_rx_start(rmt_channel, true));
      while (1) {
        // receive rmt data
        uint32_t length = 0;
        rmt_item32_t *items =
            (rmt_item32_t *)xRingbufferReceive(rb, &length, portMAX_DELAY);
        length /= sizeof(rmt_item32_t);
        ESP_LOGI(TAG, "length: %d", length);
        if (!items)
          break;
        // parse rmt data
        if (length < 4) {
          ESP_LOGI(TAG, "regarded as noise; skipping");
        } else {
          auto *p_rmt_data = new std::vector<rmt_item32_t>();
          p_rmt_data->reserve(length);
          for (int i = 0; i < length; ++i) {
            rmt_item32_t v = items[i];
            v.level0 ^= 1, v.level1 ^= 1; // invert signal levels
            p_rmt_data->push_back(v);
          }
          std::cout << "remote::rmt_raw_data_t rx = " << *p_rmt_data << ";"
                    << std::endl;
          // queue
          xQueueOverwrite(queue, &p_rmt_data);
        }
        // release rmt data
        vRingbufferReturnItem(rb, (void *)items);
      }
      ESP_LOGI(TAG, "rmt_rx_stop");
      ESP_ERROR_CHECK(rmt_rx_stop(rmt_channel));
    }

    ESP_ERROR_CHECK(rmt_driver_uninstall(rmt_channel));
    vTaskDelete(NULL);
  }
};

} // namespace remote
} // namespace app
