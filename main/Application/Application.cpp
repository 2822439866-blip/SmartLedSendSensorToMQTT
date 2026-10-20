#include "freertos/FreeRTOS.h"
#include<stdbool.h>
#include <string.h>
#include "Application.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "app_config.h"
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
}
esp_err_t Application::init()
{
    //系统状态组初始化
    sys_state_group = xEventGroupCreate();
    if(sys_state_group == nullptr){ESP_LOGE("Application::init","系统状态组创建失败"); return ESP_ERR_NO_MEM;}else{
        xEventGroupClearBits(sys_state_group, SYS_STATE_BIT_WIFI_CONNECTED | SYS_STATE_BIT_MQTT_CONNECTED);
    }

    //创建sensor和mqtt的通信消息队列   
    sensorQueue = xQueueCreate(1,sizeof(char)*50);
    if(sensorQueue == nullptr){ESP_LOGE("Application::init","传感器queue消息队列创建失败");return ESP_ERR_NO_MEM;}

    //安装中断服务
    esp_err_t gpio_install_isr_state = gpio_install_isr_service(0);
    if(gpio_install_isr_state != ESP_OK && gpio_install_isr_state != ESP_ERR_INVALID_STATE){
        ESP_LOGE("Application::init","中断服务安装失败 原因： %s",esp_err_to_name(gpio_install_isr_state));
        return gpio_install_isr_state;
    }

    //二值信号量
    this->button_semaphore = xSemaphoreCreateBinary();
    if(button_semaphore == NULL){ESP_LOGE("Application::init","按钮二值信号量创建失败");return ESP_ERR_NO_MEM;}else{
        esp_err_t gpio_isr_handler_add_state = gpio_isr_handler_add(GPIO_NUM_16,&Application::short_button_press_event,this);
        if(gpio_isr_handler_add_state != ESP_OK ){ESP_LOGE("Application::init",
                                                        "为对应的 GPIO 引脚添加 ISR 处理器失败 原因： %s",
                                                        esp_err_to_name(gpio_isr_handler_add_state));
            return gpio_isr_handler_add_state;
        }
    }

    if(this->v->getsensorListAve() < 900){this->bluleled->blink();}
    //初始化LEDC通道配置
    esp_err_t breath_y_led_tamer_init_state = breath_y_led->tamer_init();
    if(breath_y_led_tamer_init_state != ESP_OK){
        ESP_LOGE("Application::init","breath_y_led->tamer_init() 初始化失败 原因： %s",esp_err_to_name(breath_y_led_tamer_init_state));
        return breath_y_led_tamer_init_state;
    }

    //开启网络服务
    wifi = new wifiManager(sys_state_group);
    mqtt = new mqttManager(sys_state_group);
    mpu6050 = new MPU6050(sys_state_group);
    wifi->connect(APP_WIFI_SSID,APP_WIFI_PASSWORD);
    vTaskDelay(pdMS_TO_TICKS(1000)); // 等待 wifi 连接稳定
    ESP_LOGI(mqttManager::mqtt_tag, "正在连接 MQTT 服务器...");
    mqtt->mqtt_app_start();


    
    return ESP_OK;
}
void Application::Auto_led()
{   
    int light = this->v->getsensorListAve();
    if(light >=1000){this->current_state = true;
    }else if(light < 750){this->current_state = false;
    }
    if(this->last_state == true && this->current_state == false){
        //从亮变暗
        this->bluleled->blink();
        ESP_LOGE("led","开灯");
    }
    if(this->last_state == false && this->current_state == true){
        //从暗变亮
        this->bluleled->close();
        ESP_LOGE("led","关灯");
    }
    this->last_state = this->current_state;
}
void Application::breath_led_accomplish()
{

    SystemState curr_sys_state_in_breath_led_accomplish = this->get_sys_state();
    if(curr_sys_state_in_breath_led_accomplish == SystemState::sys_static){//如果静止
        //第一次开始静静态//暂停正常publsh 开启低功耗publsh
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
    case SystemState::sys_static:
        this->mpu6050->curr_sys_state_is_static = true;
        this->breath_y_led->set_safe_duty(0); //呼吸灯不亮
        break;
    case SystemState::Normal:
        this->mpu6050->curr_sys_state_is_static = false;
        // 设置占空比
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
    writeVal = writeVal/10;
    this->v->sensorList[this->count] = writeVal;
    this->count++;
    this->count = (this->count)%10;

    //发消息任务
    int light = this->v->getsensorListAve();
    char payload[50];
    int value = snprintf(payload,sizeof(payload),"{\"light_value\": %d}", light);
    if(0 < value && value < 50){//防止缓冲区溢出
        xQueueOverwrite(sensorQueue,payload);
    }else if (value >= sizeof(payload)){
        ESP_LOGE(this->mqtt->mqtt_tag, "payload 缓冲区溢出 格式化失败 被截断");
    }else {
        ESP_LOGE(this->mqtt->mqtt_tag, "payload 格式化失败");
    }       

     
}
void Application::mqtt_publish_task_logic()
{   
    //接收xQueue
    char payload[50];
    if(xQueueReceive(sensorQueue,payload,pdMS_TO_TICKS(0)) == pdTRUE){//先判断列队是否为空
        
        if((this->get_sys_state_group() & SYS_STATE_BIT_MQTT_CONNECTED) == SYS_STATE_BIT_MQTT_CONNECTED){
            this->mqtt->publish(TMQTT_PUB_TOPIC0, payload);
            ESP_LOGI(this->mqtt->mqtt_tag, "发布 MQTT 消息: %s", payload);
        }else{
            ESP_LOGW(this->mqtt->mqtt_tag, "MQTT 客户端未准备好,无法发布mqtt消息 可能是 wifi或mqtt没链接");
            ESP_LOGI(this->mqtt->mqtt_tag, "当前信息: %s", payload);
        }
    }else{
        ESP_LOGW(this->mqtt->mqtt_tag, "当前sensorQueue无数据 不可发送mqtt消息");
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
