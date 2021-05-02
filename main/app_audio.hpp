/**
 * @file app_audio.hpp
 * @author Ryotaro Onuki (kerikun11+github@gmail.com)
 * @brief i2s audio sampler
 * @date 2020-12-06
 * @copyright Copyright (c) 2020 Ryotaro Onuki
 */
#pragma once

#include "esp_log.h"

#include <memory>

#include "driver/i2s.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

namespace app {

class audio {
public:
  audio(const std::size_t item_size)
      : item_size(item_size), queue(xQueueCreate(queue_size, item_size)) {
    xTaskCreatePinnedToCore(
        [](void *_this) { reinterpret_cast<decltype(this)>(_this)->task(); },
        TAG, 4 * configMINIMAL_STACK_SIZE, this, configMAX_PRIORITIES, NULL,
        tskNO_AFFINITY);
  }
  BaseType_t receive(void *buffer, TickType_t xTicksToWait = portMAX_DELAY) {
    return xQueueReceive(queue, buffer, xTicksToWait);
  }

private:
  static constexpr char const *TAG = "record";
  static constexpr std::size_t queue_size = 2;
  const std::size_t item_size;
  QueueHandle_t queue;

  void task() {
    i2s_init();

    size_t samp_len = item_size * queue_size * sizeof(int) / sizeof(int16_t);
    auto *samp = new int[samp_len];
    size_t read_len = 0;

    while (1) {
      i2s_read(I2S_NUM_1, samp, samp_len, &read_len, portMAX_DELAY);
      for (int x = 0; x < item_size / 4; x++) {
        int s1 = ((samp[x * 4 + 0] + samp[x * 4 + 1]) >> 13) & 0x0000FFFF;
        int s2 = ((samp[x * 4 + 2] + samp[x * 4 + 3]) << 3) & 0xFFFF0000;
        samp[x] = s1 | s2;
      }
      xQueueSend(queue, samp, portMAX_DELAY);
    }

    delete[] samp;
    vTaskDelete(NULL);
  }
  static esp_err_t i2s_init(i2s_port_t i2s_port = I2S_NUM_1) {
    i2s_config_t i2s_config = {};
    i2s_config.mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX);
    i2s_config.sample_rate = 16000;
    i2s_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
    i2s_config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    i2s_config.dma_buf_count = 3;
    i2s_config.dma_buf_len = 300;
    i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL2;
    i2s_pin_config_t pin_config = {};
    pin_config.bck_io_num = 26;                  // IIS_SCLK
    pin_config.ws_io_num = 32;                   // IIS_LCLK
    pin_config.data_out_num = I2S_PIN_NO_CHANGE; // IIS_DSIN
    pin_config.data_in_num = 33;                 // IIS_DOUT
    ESP_ERROR_CHECK(i2s_driver_install(i2s_port, &i2s_config, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_pin(i2s_port, &pin_config));
    ESP_ERROR_CHECK(i2s_zero_dma_buffer(i2s_port));
    return ESP_OK;
  }
};

}; // namespace app
