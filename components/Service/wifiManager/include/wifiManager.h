#pragma once
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
class wifiManager{
   private:
      EventGroupHandle_t wifi_event_group;
      EventGroupHandle_t sys_state_group;
      int retry_num;
      static void eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

   public:
      static const char* wifi_tag;
      wifiManager(EventGroupHandle_t sys_state_group);
      void connect(const char* ssid, const char* password);
      EventBits_t get_sys_state_group();
      
};