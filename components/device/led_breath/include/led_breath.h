#pragma once
#include<stdio.h>
#include"driver/gpio.h"
#include "driver/ledc.h"
class led_breath{
    private:
        gpio_num_t pin;
        const char* name;
    public:
        led_breath(int p,const char* n,ledc_mode_t LEDC_MODE,ledc_channel_t LEDC_CHANNEL);
        void tamer_init();
        void set_safe_duty(uint32_t duty);
        void wifi_err_blink();
        void mqtt_err_blink();
        ledc_mode_t LEDC_MODE;
        ledc_channel_t LEDC_CHANNEL;
    
};