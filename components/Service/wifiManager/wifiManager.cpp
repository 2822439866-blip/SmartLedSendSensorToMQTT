#include<string.h>
#include "wifiManager.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
// WiFi 事件处理函数，负责监听 WiFi 状态的变化，并通过事件组通知主程序

// 两个信号灯标志：成功连上给第 0 位（BIT0），彻底失败给第 1 位（BIT1）
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define MAX_RETRY          5

#define SYS_STATE_BIT_WIFI_CONNECTED BIT0
#define SYS_STATE_BIT_MQTT_CONNECTED BIT1
#define SYS_STATE_BIT_MOVING BIT2
#define SYS_STATE_BIT_ALL_NORMAL (SYS_STATE_BIT_WIFI_CONNECTED | SYS_STATE_BIT_MQTT_CONNECTED | SYS_STATE_BIT_MOVING)

const char* wifiManager::wifi_tag = "wifi";
void wifiManager::eventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    wifiManager* wifi = (wifiManager*)arg;

    //硬件准备好了”
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect(); // 让天线真正去和路由器对暗号
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGE(wifi_tag, "⚠️ diss dissconnected: %d", event->reason);
        // 更新系统状态 呼吸灯会感知到这个状态并改变自己的行为
        xEventGroupClearBits(wifi->sys_state_group, SYS_STATE_BIT_WIFI_CONNECTED); // 更新系统状态
        esp_wifi_connect();//不管超过多少次失败 我们都让他重新连接
        if (wifi->retry_num < MAX_RETRY) {
            //esp_wifi_connect(); // 没超过 5 次，继续尝试重连
            wifi->retry_num++;
            ESP_LOGW(wifi_tag, "断开或未连上，正在进行第 %d 次自动重连...",wifi->retry_num);
        } else {
            // 超过 5 次都连不上，彻底放弃，给正在死等的主程序点亮【失败红灯】（BIT1）
            xEventGroupSetBits(wifi->wifi_event_group, WIFI_FAIL_BIT);//点灯 通知在connect函数死等的程序
        }
    } 
    // 分支 C：高潮！总机狂喊：“成功拿到路由器分配的门牌号（IP_EVENT_STA_GOT_IP）了！”
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        // 打印出让人兴奋的绿色 IP 日志
        ESP_LOGI(wifi_tag, "IP : " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi->sys_state_group,SYS_STATE_BIT_WIFI_CONNECTED); // 更新系统状态
        wifi->retry_num = 0; // 重置计数器
        // 给正在死等的主程序点亮【成功绿灯】（BIT0），主程序瞬间苏醒！
        xEventGroupSetBits(wifi->wifi_event_group, WIFI_CONNECTED_BIT);
    }
}
wifiManager::wifiManager(EventGroupHandle_t sys_state_group)
{    
    wifiManager::sys_state_group = sys_state_group;//这是系统的状态
   //WiFi硬件每次启动都要去Flash读射频参数 必须先初始化 NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI("MAIN", "正在实例化WiFi管理器");
    retry_num = 0;
    wifi_event_group = xEventGroupCreate(); // 创建是否成功连接到wifi的事件组
    
    //初始化TCP/IP
    ESP_ERROR_CHECK(esp_netif_init()); 
    ESP_ERROR_CHECK(esp_event_loop_create_default()); //开启后台总机任务
    esp_netif_create_default_wifi_sta();              //让ESP32 准备当一个接收端
    // 初始化 WiFi硬件驱动
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 注册回调函数
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &eventHandler, this, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &eventHandler, this, NULL);
}
void wifiManager::connect(const char *ssid, const char *password)
{
    //账号密码填写给底层 握手
    wifi_config_t wifi_config = {}; 
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);//strncpy 函数要求的参数类型是：有符号字符指针 所以要把uint8_t的wifi_config.sta.ssid转成char*
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK; // 指定加密方式
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

EventBits_t wifiManager::get_sys_state_group()
{   
    EventBits_t sys_state_bit = xEventGroupGetBits(this->sys_state_group);
    return sys_state_bit;
}
