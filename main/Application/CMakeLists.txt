idf_component_register(SRCS "Application.cpp"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES esp_common
                       REQUIRES SmartLed Sensor led_breath Button mqttManager wifiManager esp_timer taskManager MPU6050 app_config)
