#include "freertos/FreeRTOS.h"


#include "SmartLed.h"
#include "Sensor.h"
#include "Button.h"
#include "led_breath.h"
#include "mqttManager.h"
#include"wifiManager.h"
#include "Application.h"
#include "taskManager.h"
//static const char* led_tag = "LED_Class";
Application* app = nullptr;
taskManager* my_task = nullptr;


extern "C" void app_main(void)    
{   
    app = Application::GetInstance();
    app->init();

    my_task = new taskManager(app);
    my_task->task_create();

}
