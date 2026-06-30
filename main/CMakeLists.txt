idf_component_register(SRCS "SmartLedSendSensorToMQTTexternC.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES SmartLed Sensor led_breath Button mqttManager wifiManager Application taskManager)
