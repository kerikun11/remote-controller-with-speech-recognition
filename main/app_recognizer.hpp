#pragma once

#include "esp_log.h"

#include <memory>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_mn_models.h"
#include "esp_wn_models.h"

#include "app_audio.hpp"

namespace app {

class recognizer {
public:
  enum EventId {
    WOKE_UP,
    GOT_COMMAND,
  };
  struct Event {
    EventId id;
    int command_id;
  };

public:
  recognizer() {
    queue = xQueueCreate(10, sizeof(Event));
    xTaskCreatePinnedToCore(
        [](void *_this) { reinterpret_cast<decltype(this)>(_this)->task(); },
        TAG, 4 * configMINIMAL_STACK_SIZE, this, configMAX_PRIORITIES, NULL,
        tskNO_AFFINITY);
  }
  BaseType_t receive(Event &event, TickType_t xTicksToWait = portMAX_DELAY) {
    return xQueueReceive(queue, &event, xTicksToWait);
  }

private:
  static constexpr char const *TAG = "recognizer";
  QueueHandle_t queue;
  // WakeNet
  const esp_wn_iface_t *wakenet = &WAKENET_MODEL;
  model_iface_data_t *model_data_wn = NULL;
  // MultiNet
  const esp_mn_iface_t *multinet = &MULTINET_MODEL;
  model_iface_data_t *model_data_mn = NULL;

  void task() {
    // WakeNet
    model_data_wn = wakenet->create(&WAKENET_COEFF, DET_MODE_95);
    // MultiNet
    model_data_mn = multinet->create(&MULTINET_COEFF, 3000);

    // info
    const int wake_word_num = wakenet->get_word_num(model_data_wn);
    const char *wake_word_list = wakenet->get_word_name(model_data_wn, 1);
    if (wake_word_num)
      ESP_LOGI(TAG, "wake word number = %d, word1 name = %s", wake_word_num,
               wake_word_list);

    // prepare audio source
    int audio_chunksize = wakenet->get_samp_chunksize(model_data_wn);
    audio audio(audio_chunksize * sizeof(int16_t));

    int chunk_num = multinet->get_samp_chunknum(model_data_mn);
    ESP_LOGI(TAG, "chunk_num = %d", chunk_num);

    // alloc
    int16_t *buffer = (int16_t *)malloc(audio_chunksize * sizeof(int16_t));
    assert(buffer);
    int chunks = 0;
    int mn_chunks = 0;
    bool detect_flag = 0;
    int frequency = wakenet->get_samp_rate(model_data_wn);

    while (1) {
      audio.receive(buffer);

      if (detect_flag == 0) {
        int r = wakenet->detect(model_data_wn, buffer);
        if (r) {
          float ms = (chunks * audio_chunksize * 1000.0) / frequency;
          ESP_LOGI(TAG, "%.2f: %s DETECTED.", (float)ms / 1000.0,
                   wakenet->get_word_name(model_data_wn, r));
          detect_flag = 1;
          Event event;
          event.id = EventId::WOKE_UP;
          xQueueSend(queue, &event, portMAX_DELAY);
          ESP_LOGI(TAG, "-----------------LISTENING-----------------");
        }
      } else {
        int command_id = multinet->detect(model_data_mn, buffer);
        mn_chunks++;
        if (mn_chunks == chunk_num || command_id > -1) {
          mn_chunks = 0;
          detect_flag = 0;
          if (command_id > -1) {
            ESP_LOGI(TAG, "Commands ID: %d.", command_id);
          } else {
            ESP_LOGI(TAG, "can not recognize any speech commands");
          }
          Event event;
          event.id = EventId::GOT_COMMAND;
          event.command_id = command_id;
          xQueueSend(queue, &event, portMAX_DELAY);
          ESP_LOGI(TAG, "-----------awaits to be waken up-----------");
        }
      }
      chunks++;
    }

    wakenet->destroy(model_data_wn);
    multinet->destroy(model_data_mn);
    free(buffer);
    vTaskDelete(NULL);
  }
};

}; // namespace app
