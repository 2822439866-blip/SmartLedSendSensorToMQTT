#include"led_breath.h"
#include "freertos/FreeRTOS.h"
#include "driver/ledc.h"
led_breath::led_breath(int p,const char* n,ledc_mode_t LEDC_MODE,ledc_channel_t LEDC_CHANNEL){
    pin = (gpio_num_t)p;
    name = n;
    this->LEDC_MODE = LEDC_MODE;
    this->LEDC_CHANNEL = LEDC_CHANNEL;//类定义的参数 传入的参数必须显示赋值！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
}
void led_breath::tamer_init(){
    //定时器配置
    ledc_timer_config_t ledc_timer = {};
    ledc_timer.speed_mode = LEDC_MODE;//定时器使用低速模式 省电
    ledc_timer.duty_resolution = LEDC_TIMER_12_BIT;//位精度12位 0~2^12 
    ledc_timer.timer_num = LEDC_TIMER_0;//使用定时器0 （啥都可以）要和PWM通道配置的定时器编号一致    
    ledc_timer.freq_hz = 5000;//频率                     
    ledc_timer.clk_cfg = LEDC_AUTO_CLK; //LEDC 定时器的时钟源选择 决定 PWM 频率从哪个时钟分频而来                  
    ledc_timer_config(&ledc_timer);       
    
    // 通道配置//因为我们之前配置了定时器 选择了LEDC_TIMER_0 所以要再pwm通道应用他们才能绑定在一起
    ledc_channel_config_t ledc_channel = {};
    ledc_channel.gpio_num = pin;//给哪个引脚输出PWM信号 要和实际连接LED的引脚一致
    ledc_channel.speed_mode = LEDC_MODE;//必须要和定时器配置的速度模式一致
    ledc_channel.channel = LEDC_CHANNEL;//我们用的是第0PWM通道输出
    ledc_channel.intr_type = LEDC_INTR_DISABLE;//不使用中断
    ledc_channel.timer_sel = LEDC_TIMER_0;//应用之前配置的定时器0
    ledc_channel.duty = 0;//初始占空比为0（LED熄灭）
    ledc_channel_config(&ledc_channel);
}
void led_breath::wifi_err_blink(){//闪一下 快速闪两下
    this->set_safe_duty(4095);
    vTaskDelay(pdMS_TO_TICKS(200));
    this->set_safe_duty(0);

    vTaskDelay(pdMS_TO_TICKS(400));

    this->set_safe_duty(4095);
    vTaskDelay(pdMS_TO_TICKS(200));
    this->set_safe_duty(0);
    vTaskDelay(pdMS_TO_TICKS(200));
    this->set_safe_duty(4095);
    vTaskDelay(pdMS_TO_TICKS(200));
    this->set_safe_duty(0);

    vTaskDelay(pdMS_TO_TICKS(1000));
}
void led_breath::mqtt_err_blink(){//一秒闪一次
    this->set_safe_duty(4095);
    vTaskDelay(pdMS_TO_TICKS(200));
    this->set_safe_duty(0);
    vTaskDelay(pdMS_TO_TICKS(800));
}
void led_breath::set_safe_duty(uint32_t duty){
    ledc_set_duty(this->LEDC_MODE, this->LEDC_CHANNEL, duty);
    ledc_update_duty(this->LEDC_MODE, this->LEDC_CHANNEL);
}
    
