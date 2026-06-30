idf_component_register(SRCS "Application.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES SmartLed Sensor led_breath Button mqttManager wifiManager esp_timer taskManager MPU6050)
