#include "freertos/FreeRTOS.h"

#include<stdbool.h>
#include "Application.h"
#include "esp_log.h"
#include "esp_timer.h"
//系统状态事件组
#define SYS_STATE_BIT_WIFI_CONNECTED BIT0
#define SYS_STATE_BIT_MQTT_CONNECTED BIT1
#define SYS_STATE_BIT_MOVING BIT2
#define SYS_STATE_BIT_ALL_NORMAL (SYS_STATE_BIT_WIFI_CONNECTED | SYS_STATE_BIT_MQTT_CONNECTED | SYS_STATE_BIT_MOVING)

const char* Application::isr_tag = nullptr;
bool Application::last_state;
bool Application::current_state;
SystemState Application::sys_state;
Application::Application()
{
    breath_y_led = new led_breath(GPIO_NUM_4, "黄色LED呼吸灯",LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    breath_y_led->tamer_init();
    v = new Sensor(32, "电压传感器");
    bluleled = new SmartLed(GPIO_NUM_2, "blueLED", 500, 500);
    fastbutton = new Button(GPIO_NUM_16, "Button");
    wifi = nullptr;
    mqtt = nullptr;

    Application::sys_state_group = NULL;//全局事件组句柄

    this->last_time = 0;
    isr_tag = "task";
            
    last_state = false;
    current_state = false;
    sys_state = Normal;
    sensorQueue = xQueueCreate(10,sizeof(char)*50);
    
}
void Application::Auto_led()
{   
    int light = this->v->getsensorListAve();
    if(light >=1000){this->current_state = true;
    }else if(light < 750){this->current_state = false;
    }
    if(this->last_state == true && this->current_state == false){
        //从亮变暗了
        this->bluleled->blink();
    }
    if(this->last_state == false && this->current_state == true){
        //从暗变亮了
        this->bluleled->close();
    }
    this->last_state = this->current_state;
}
void Application::breath_led_accomplish()
{
    /*if(last_sys_state == SYS_STATE_NORMAL && sys_state != SYS_STATE_NORMAL){//如果是 正常模式->非正常模式 的话 要开闪烁模式 就要把原先的PWM输出模式改成gpio输出模式//引脚被 PWM 硬件控制器霸占， gpio_set_level 完全控制不了
            breath_y_led->led_pwm_to_gpio();
        }else if(last_sys_state != SYS_STATE_NORMAL && sys_state == SYS_STATE_NORMAL){//非正常模式 -> 正常模式
            breath_y_led->led_gpio_to_pwm();
        }
            当你在初始化时调用了官方的 ledc_channel_config，ESP32 底层会将物理引脚（GPIO 4）通过内部的 GPIO 矩阵（GPIO Matrix） 强行硬连线到 PWM 外设信号上。
            当你断开 Wi-Fi 触发错误，代码跑去执行 gpio_reset_pin 和 gpio_config 时，虽然把引脚的方向改成了“输出模式”，但是！它并没有去切断 GPIO 矩阵与 PWM 硬件之间的物理连线！ 
            引脚依然被硬性绑定在刚才停止的 LEDC 通道上。无论你在 wifi_err_blink() 里如何调用 gpio_set_level(pin, 1) 或 0，物理引脚根本听不到 GPIO 的命令，它依然保持着被锁死在
            LEDC 停止时的那个电平（也就是你看到的常亮）。*/
            //172 && 175行切换使用pwm 或 gpio输出函数无效 函数无用 因为引脚被 PWM 硬件控制器霸占， gpio_set_level 完全控制不了

        //获取当前系统状态
        SystemState curr_sys_state_in_breath_led_accomplish = this->get_sys_state();

        if(curr_sys_state_in_breath_led_accomplish == SystemState::sys_static){          //如果静止
            //第一次开始静静态
            this->mpu6050->curr_sys_state_is_static = true;
            if(this->mpu6050->last_sys_state_is_static == false && this->mpu6050->curr_sys_state_is_static == true){
                ESP_LOGW(this->mpu6050->MPU6050_tag,"检测到系统长时间静止，开启低功耗模式");
                vTaskSuspend(publsh_handl);
                vTaskResume(low_power_publsh_hanel);
            }

        }else{//不静止
            this->mpu6050->curr_sys_state_is_static = false;
            if(this->mpu6050->last_sys_state_is_static == true && this->mpu6050->curr_sys_state_is_static == false){
                ESP_LOGW(this->mpu6050->MPU6050_tag,"检测到系统移动，关闭低功耗模式");
                vTaskSuspend(low_power_publsh_hanel);
                vTaskResume(publsh_handl);
            }

        }

        switch (curr_sys_state_in_breath_led_accomplish)
        {
        case SystemState::WifiError:
            this->mpu6050->curr_sys_state_is_static = false;
            ESP_LOGE(this->wifi->wifi_tag, "WiFi 断开了...");
            //WiFi断开错误模式
            this->breath_y_led->wifi_err_blink();
            break;
        case SystemState::MqttError:
            this->mpu6050->curr_sys_state_is_static = false;
            ESP_LOGE(this->mqtt->mqtt_tag, "MQTT 断开了...但是wifi是好的");
            //MQTT断开错误模式
            this->breath_y_led->mqtt_err_blink();
            break;
        case SystemState::sys_static://降低功耗逻辑
            //1.呼吸灯不亮2.publsh频率变慢
            //1.
            this->mpu6050->curr_sys_state_is_static = true;
            this->breath_y_led->set_safe_duty(0);
            //暂停正常publsh 开启低功耗publsh
            break;
        case SystemState::Normal:
            this->mpu6050->curr_sys_state_is_static = false;
             // 设置占空比
            //ledc_set_duty(breath_y_led->LEDC_MODE, breath_y_led->LEDC_CHANNEL, duty);//是非原子、非线程安全的两步操作，多核 / 多任务下极易冲突
            //ledc_update_duty(breath_y_led->LEDC_MODE, breath_y_led->LEDC_CHANNEL);//是非原子、非线程安全的两步操作，多核 / 多任务下极易冲突
            this->breath_y_led->set_safe_duty(this->duty);//我们自己封装的线程安全函数，内部用自旋锁保护了占空比设置和更新的原子操作
            // 调整亮度
            this->duty += this->fade;
            // 到达边界反转方向
            if (this->duty >= 4095) {
                this->fade = -20;   
            } else if (this->duty <= 0) {
                this->fade = 20;   
            }

            break;
        default:
            break;
        }
        this->mpu6050->last_sys_state_is_static = this->mpu6050->curr_sys_state_is_static;
}
void Application::sensorlist_task_accomplish()
{   //任务每一次执行都取10次平均，放入sensorlist里面
    int writeVal = 0;
    for(int i=0;i<=9;i++){
            writeVal += this->v->readValue();
    }
    this->v->sensorList[this->count] = writeVal/10;
    this->count++;
    this->count = (this->count)%10;
        

    //发消息任务
    int light = this->v->getsensorListAve();
    char payload[50];
    sprintf(payload, "{\"light_value\": %d}", light);
    xQueueSend(sensorQueue,payload,pdMS_TO_TICKS(0));
    
    
}
void Application::mqtt_publish_task_logic()
{   
    //接收xQueue
    char payload[50];
    if(xQueueReceive(sensorQueue,payload,pdMS_TO_TICKS(0)) == pdTRUE){
        if((this->get_sys_state_group() & SYS_STATE_BIT_MQTT_CONNECTED) == SYS_STATE_BIT_MQTT_CONNECTED){
            this->mqtt->publish("/esp32/cym/sensor/light", payload);
             ESP_LOGI(this->mqtt->mqtt_tag, "发布 MQTT 消息: %s", payload);
        }else{
            ESP_LOGW(this->mqtt->mqtt_tag, "MQTT 客户端未准备好，无法发布消息 可能是 wifi或mqtt没链接");
            ESP_LOGI(this->mqtt->mqtt_tag, "当前信息: %s", payload);
        }
    }
    
}



void IRAM_ATTR Application::short_button_press_event(void* arg)
{   
    Application* app = (Application*) arg;
    int64_t curr_time = esp_timer_get_time();
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if(curr_time - app->last_time <180*1000){app->last_time = curr_time;return;}
    xSemaphoreGiveFromISR(app->button_semaphore,&xHigherPriorityTaskWoken);
    if(xHigherPriorityTaskWoken == pdTRUE){
        portYIELD_FROM_ISR();
    }
    app->last_time = curr_time;
}

void Application::init()
{
    //系统状态组
    sys_state_group = xEventGroupCreate();
    xEventGroupClearBits(sys_state_group, SYS_STATE_BIT_WIFI_CONNECTED | SYS_STATE_BIT_MQTT_CONNECTED);

    //安装中断服务
    gpio_install_isr_service(0);;
    //二值信号量
    this->button_semaphore = xSemaphoreCreateBinary();
    gpio_isr_handler_add(GPIO_NUM_16,&Application::short_button_press_event,this);


    wifi = new wifiManager(sys_state_group);
    mqtt = new mqttManager(sys_state_group);
    mpu6050 = new MPU6050(sys_state_group);
    wifi->connect("Katatumuri","000999888");
    vTaskDelay(pdMS_TO_TICKS(1000)); // 等待 wifi 连接稳定
    ESP_LOGI(mqttManager::mqtt_tag, "正在连接 MQTT 服务器...");
    mqtt->mqtt_app_start();

    if(this->v->getsensorListAve() > 900){this->bluleled->blink();}

}

EventBits_t Application::get_sys_state_group()
{   
    EventBits_t wifi_state = this->wifi->get_sys_state_group();
    EventBits_t mqtt_state = this->mqtt->get_sys_state_group();
    EventBits_t moving_state = this->mpu6050->get_sys_state_group();
    xEventGroupSetBits(sys_state_group,wifi_state|mqtt_state|moving_state);
    return xEventGroupGetBits(sys_state_group);
}

SystemState Application::get_sys_state()
{
    EventBits_t curr_sys_state_group = this->get_sys_state_group();
    if((curr_sys_state_group & SYS_STATE_BIT_WIFI_CONNECTED) == 0){
        sys_state = WifiError;
    }else if((curr_sys_state_group & SYS_STATE_BIT_MQTT_CONNECTED) == 0){
        sys_state = MqttError;
    }else if((curr_sys_state_group & SYS_STATE_BIT_MOVING) == 0){//没有正在动 静止状态 降低功耗
        sys_state = sys_static;
    }else{
        sys_state = Normal;
    }
    return sys_state;
}
