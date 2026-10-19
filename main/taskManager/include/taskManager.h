#pragma once
#include "freertos/FreeRTOS.h"

#include "freertos/queue.h"
#include "Application.h"

#define SYS_STATE_BIT_WIFI_CONNECTED BIT0
#define SYS_STATE_BIT_MQTT_CONNECTED BIT1
#define SYS_STATE_BIT_ALL_NORMAL (SYS_STATE_BIT_WIFI_CONNECTED | SYS_STATE_BIT_MQTT_CONNECTED)


class taskManager{
    public:
        taskManager(Application* app);
        BaseType_t task_create();
        Application* app; 
        TaskHandle_t sensorlist_task_Handle = nullptr;
        TaskHandle_t led_breath_task_Handle = nullptr;
        TaskHandle_t MPU6050_out_put_Handle= nullptr;
        TaskHandle_t short_button_task_Handle = nullptr;
        TaskHandle_t led_control_task_Handle = nullptr;
        TaskHandle_t mqtt_publish_task_handl = nullptr;
        TaskHandle_t mqtt_low_power_publish_task_handl = nullptr;
    private:
        //任务
        static void short_button_task(void *pvParameter);
        static void mqtt_publish_task(void *pvParameter);
        static void mqtt_publish_task_low_power(void *pvParameter);
        static void led_control_task(void *pvParameter);
        static void sensorlist_task(void *pvParameter);
        static void led_breath_task(void *pvParameter);
        static void MPU6050_out_put(void *pvParameter);
        

};