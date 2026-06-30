#pragma once
#include<stdio.h>
#include "mqtt_client.h"
#include "freertos/event_groups.h"
class mqttManager{
   private:
      esp_mqtt_client_handle_t client = NULL;//订阅发布时所需要的句柄
      EventGroupHandle_t sys_state_group;//mqqt事件组
      static void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data);
   public:
      EventBits_t get_sys_state_group();
      static const char* mqtt_tag;

      mqttManager(EventGroupHandle_t sys_state_group);
      void publish(const char* topic, const char* payload);
      void subscribe(const char* topic);
      void mqtt_app_start(void);//开始链接mqqt对外接口
      
};
