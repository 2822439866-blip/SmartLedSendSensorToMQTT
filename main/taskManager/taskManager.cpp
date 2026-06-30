
#include "taskManager.h"
#include "Application.h"
#include "esp_log.h"
#include "esp_timer.h"




void taskManager::led_breath_task(void *pvParameter){
    //sys_state_t last_sys_state = SYS_STATE_NORMAL;//存档 加上边缘检测 避免一直接换输出模式 占用锁 一直写入寄存器 导致看门狗崩溃//由于下行注释原因 这个变量我们暂且不需要
    taskManager *Manager = (taskManager*)pvParameter;
    //12位分辨率的占空比范围是 0~4095
    while(1){
        Manager->app->breath_led_accomplish();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void taskManager::MPU6050_out_put(void *pvParameter)
{
    taskManager *Manager = (taskManager*)pvParameter;
    while(1){
        Manager->app->mpu6050->put_data_in_datalist();
        Manager->app->mpu6050->calculate_ave_and_s();
        Manager->app->mpu6050->change_sys_state();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void taskManager::sensorlist_task(void *pvParameter) //每100ms更新一次平均值
{   taskManager *Manager = (taskManager*)pvParameter;
    while(1){
        Manager->app->sensorlist_task_accomplish();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
void taskManager::led_control_task(void *pvParameter){//自动根据光照强度调整LED状态
    taskManager *Manager = (taskManager*)pvParameter;
    while(1){
        Manager->app->Auto_led();//auto_led()函数的意义就是天黑天亮自动关灯轮询 但是唯一的问题是 除了auto_led()函数控制led亮与灭 与此同时还有中断的short_button 当short_button改变状态 auto_led()又回变回来 导致冲突 我就加上了 curr_state and last_tate 边缘判断机制 避免冲突
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void taskManager::mqtt_publish_task(void *pvParameter)//每1秒发布一次光照强度
{   
    taskManager *Manager = (taskManager*)pvParameter;
    while(1){
        Manager->app->mqtt_publish_task_logic();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void taskManager::mqtt_publish_task_low_power(void *pvParameter)
{
    taskManager *Manager = (taskManager*)pvParameter;
    while(1){
        Manager->app->mqtt_publish_task_logic();
        vTaskDelay(pdMS_TO_TICKS(1000*60));//一分钟发一次
    }
}

void taskManager::short_button_task(void *pvParameter){
    taskManager *Manager = (taskManager*)pvParameter;
    while(1){//必须要写死循环，否则这个函数执行完了就结束了，按钮中断服务函数必须要一直存在才能响应中断
        if(xSemaphoreTake(Manager->app->button_semaphore,portMAX_DELAY) == pdTRUE){
            ESP_LOGI(Manager->app->fastbutton->button_tag, "按钮被按下 中断触发");
            if(Manager->app->bluleled->state == false){Manager->app->bluleled->blink();
            }else{
                Manager->app->bluleled->close();
            }
        }

    }
}

taskManager::taskManager(Application* application)
{   
    this->app = application;
    
}
void taskManager::task_create()
{
    xTaskCreate(sensorlist_task, "Sensor_Task", 4096, this, 5, NULL);
    xTaskCreate(led_control_task, "led_control_task", 4096, this, 5, NULL);
    xTaskCreate(mqtt_publish_task, "MQTT_Queue_Task", 4096, this, 5, &mqtt_publish_task_handl);
    xTaskCreate(short_button_task, "short_button_task", 4096, this, 5, NULL);
    xTaskCreate(led_breath_task, "led_breath_task", 4096, this, 4, NULL);
    xTaskCreate(MPU6050_out_put, "MPU6050_out_put", 4096, this, 4, NULL);

    xTaskCreate(mqtt_publish_task_low_power, "mqtt_publish_task_low_power", 4096, this, 5, &mqtt_low_power_publish_task_handl);
    vTaskSuspend(mqtt_low_power_publish_task_handl);
    //任务创建完之后 给application传入任务句柄 在application中通过句柄控制publsh模式
    app->publsh_handl = mqtt_publish_task_handl;
    app->low_power_publsh_hanel = mqtt_low_power_publish_task_handl;
}
