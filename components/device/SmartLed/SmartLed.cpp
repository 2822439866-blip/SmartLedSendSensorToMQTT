#include"SmartLed.h"
#include "freertos/FreeRTOS.h"

SmartLed::SmartLed(gpio_num_t p, const char *n, int ont, int offt)
{
   pin = p;
   name = n;
   brightTime = ont;
   onbrightTime = offt;
   state = false;
   //重置引脚
   gpio_reset_pin(pin);
   //详细配置引脚
   gpio_config_t io_conf = {};
   io_conf.pin_bit_mask = (1ULL << this->pin);//选择引脚  
   io_conf.mode = GPIO_MODE_OUTPUT;//设置为输出
   io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
   io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
   io_conf.intr_type = GPIO_INTR_DISABLE;//关闭中断   
   gpio_config(&io_conf);
   gpio_set_level(pin, 0); //默认熄灭

}
void SmartLed::blink()
{
    gpio_set_level(pin, 1);
    state = true;

}
void SmartLed::flash(){
    gpio_set_level(pin, 1);
    state = true;
    vTaskDelay(pdMS_TO_TICKS(brightTime));
    gpio_set_level(pin, 0);
    state = false;
    vTaskDelay(pdMS_TO_TICKS(onbrightTime));
}
void SmartLed::close(){
    gpio_set_level(pin, 0);
    state = false;
}
