#include "mqttManager.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "mqtt_client.h"

#define SYS_STATE_BIT_WIFI_CONNECTED BIT0
#define SYS_STATE_BIT_MQTT_CONNECTED BIT1
#define SYS_STATE_BIT_MOVING BIT2
#define SYS_STATE_BIT_ALL_NORMAL (SYS_STATE_BIT_WIFI_CONNECTED | SYS_STATE_BIT_MQTT_CONNECTED | SYS_STATE_BIT_MOVING)

const char* mqttManager::mqtt_tag = "MQTT_Class";
// MQTT 事件处理函数
void mqttManager::mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{  
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    mqttManager* mqtt= (mqttManager*)handler_args;
    ESP_LOGD(mqtt->mqtt_tag, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);

    //esp_mqtt_event_handle_t* event = event_data;//这行代码是 esp_mqtt_event_handle_t event = event_data;。这里的 event_data 是系统传过来的万能指针 void*。在 C 语言里，void* 可以随便变成任何指针；但在 C++ 里，必须显式强转。wifiManager* myWifi = (wifiManager*)arg;


    int msg_id = event->msg_id;
    switch ((esp_mqtt_event_id_t)event_id){
    case MQTT_EVENT_CONNECTED:
        xEventGroupSetBits(mqtt->sys_state_group,SYS_STATE_BIT_MQTT_CONNECTED); // 更新系统状态，呼吸灯会感知到这个状态并改变自己的行为
        ESP_LOGI(mqtt_tag, "MQTT_EVENT_CONNECTED");
        break;
    case MQTT_EVENT_DISCONNECTED:
        xEventGroupClearBits(mqtt->sys_state_group,SYS_STATE_BIT_MQTT_CONNECTED); // 更新系统状态
        ESP_LOGW(mqtt_tag, "MQTT_EVENT_DISCONNECTED");
        ESP_LOGI(mqtt_tag, "链接断开了...");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(mqtt_tag, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", msg_id);
        ESP_LOGI(mqtt_tag, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED://取消订阅成功
        ESP_LOGI(mqtt_tag, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(mqtt_tag, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(mqtt_tag, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(mqtt_tag, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGE(mqtt_tag, "reported from esp-tls: %d", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGE(mqtt_tag, "reported from tls stack: %d", event->error_handle->esp_tls_stack_err);
            ESP_LOGE(mqtt_tag, "captured as transport's socket errno: %d", event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(mqtt_tag, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(mqtt_tag, "Other event id:%d", event->event_id);
        break;
    }
}
mqttManager::mqttManager(EventGroupHandle_t sys_state_group)
{   
    mqttManager::sys_state_group= sys_state_group;
    ESP_LOGI(mqtt_tag, "mqtt准备中");
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);
}
void mqttManager::publish(const char *topic, const char *payload)
{
    esp_mqtt_client_publish(client, topic, payload, 0, 0, 0);
}
void mqttManager::subscribe(const char *topic)
{
    esp_mqtt_client_subscribe(client, topic, 0);
}
void mqttManager::mqtt_app_start()
{  
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = "mqtt://broker.emqx.io";
    client = esp_mqtt_client_init(&mqtt_cfg);//在这个函数里面给私有变量client赋值了，必须调用这个函数来确保client被正确初始化
    esp_mqtt_client_register_event(client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, this);
    esp_mqtt_client_start(client);
}

EventBits_t mqttManager::get_sys_state_group()
{
    EventBits_t sys_state_bit = xEventGroupGetBits(this->sys_state_group);
    return sys_state_bit;
}
