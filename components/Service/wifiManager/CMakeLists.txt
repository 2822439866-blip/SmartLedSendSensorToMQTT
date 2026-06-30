idf_component_register(SRCS "wifiManager.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES freertos esp_wifi log esp_event nvs_flash)