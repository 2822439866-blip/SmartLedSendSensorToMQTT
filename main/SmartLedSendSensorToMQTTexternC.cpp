#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "esp_log.h"

#include "SmartLed.h"
#include "Sensor.h"
#include "Button.h"
#include "led_breath.h"
#include "mqttManager.h"
#include"wifiManager.h"
#include "Application.h"
#include "taskManager.h"

Application* app = nullptr;
taskManager* my_task = nullptr;


extern "C" void app_main(void)    
{   
    app = Application::GetInstance();
    esp_err_t os_init_state = app->init();
    if(os_init_state != ESP_OK){
        ESP_LOGE("app_main","Application::init() 初始化失败 原因： %s",esp_err_to_name(os_init_state));
        while(1){
            vTaskDelay(pdMS_TO_TICKS(1000*10));
        }
    }
    my_task = new taskManager(app);
    if(my_task->task_create() == pdPASS){
        ESP_LOGI("taskManager::task_create()","task创建成功");
    }else{
        ESP_LOGE("taskManager::task_create()", "task创建失败");
        while(1){
            vTaskDelay(pdMS_TO_TICKS(1000*10));
        }
    }
    vTaskDelay(pdMS_TO_TICKS(1000*90));
    ESP_LOGW("MAIN_task_create Mamery ","sensorlist_task high lavel %d",uxTaskGetStackHighWaterMark(my_task->sensorlist_task_Handle));
    ESP_LOGW("MAIN_task_create Mamery ","led_breath_task high lavel %d",uxTaskGetStackHighWaterMark(my_task->led_breath_task_Handle));
    ESP_LOGW("MAIN_task_create Mamery ","MPU6050_out_put high lavel %d",uxTaskGetStackHighWaterMark(my_task->MPU6050_out_put_Handle));
    ESP_LOGW("MAIN_task_create Mamery ","short_button_task high lavel %d",uxTaskGetStackHighWaterMark(my_task->short_button_task_Handle));
    ESP_LOGW("MAIN_task_create Mamery ","led_control_task high lavel %d",uxTaskGetStackHighWaterMark(my_task->led_control_task_Handle));
    ESP_LOGW("MAIN_task_create Mamery ","mqtt_publish_task high lavel %d",uxTaskGetStackHighWaterMark(my_task->mqtt_publish_task_handl));
    ESP_LOGW("MAIN_task_create Mamery ","mqtt_low_power_publish_task high lavel %d",uxTaskGetStackHighWaterMark(my_task->mqtt_low_power_publish_task_handl));
}
