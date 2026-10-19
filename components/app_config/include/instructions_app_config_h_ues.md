## APP_WIFI_SSID & APP_WIFI_PASSWORD
使用app_config.h配置账号密码 app在系统初始化阶段链接 [esp_err_t Application::init() : wifi->connect(.. , ..)]

## BOKER_ADDRESS_URI & TOPIC*
使用app_config.h配置MQTT与服务器链接 
uri链接位置：[esp_err_t Application::init() : mqtt->mqtt_app_start();]
boker topic 在 taskManager中调用mqtt_publish_task里mqtt_publish_task_logic具体发送topic[void Application::mqtt_publish_task_logic()]
