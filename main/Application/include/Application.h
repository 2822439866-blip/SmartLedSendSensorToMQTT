// 定义系统的全局运行状态
#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "mqttManager.h"
#include "wifiManager.h"
#include "SmartLed.h"
#include "Sensor.h"
#include "led_breath.h"
#include "Button.h"
#include "MPU6050.h"



enum SystemState {
    Init,           
    Normal,         
    WifiConnected, 
    WifiError,      
    MqttConnecting, 
    MqttError,
    sys_static,
    sys_moveing //正常状态运行      
};
class Application{
    public:
        // 获取单例实例 (整个系统只有一个 Application)
        static Application* GetInstance() {
            static Application instance;//先是：因为有static 所以分配空间后 全部是nullptr//然后：运行类的构造函数 填数据 就不是空壳
            return &instance;
        }
        

        Button* fastbutton;
        SmartLed* bluleled;
        Sensor* v;
        led_breath* breath_y_led;
        mqttManager* mqtt;
        wifiManager* wifi;
        MPU6050* mpu6050;

        EventGroupHandle_t sys_state_group;
        SemaphoreHandle_t button_semaphore;//中断

        TaskHandle_t publsh_handl = nullptr;
        TaskHandle_t low_power_publsh_hanel = nullptr;
        void init();
        SystemState get_sys_state();

        //业务判断逻辑&控制底层操作
        void Auto_led();
        void breath_led_accomplish();
        void sensorlist_task_accomplish();
        void mqtt_publish_task_logic();
        
    private:
        Application();
        EventBits_t get_sys_state_group();
        static void IRAM_ATTR short_button_press_event(void* arg);
        static enum SystemState sys_state;
        int64_t last_time;
        // 禁用拷贝构造和赋值//避免破坏单例约束
        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;
        static const char* isr_tag;        
        static bool last_state;
        static bool current_state;
        QueueHandle_t sensorQueue; 
        int duty = 0;
        int fade = 20; 
        int sensor_list_count = 0;
        int count = 0;
};