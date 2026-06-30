#include "Button.h"
const char* Button::button_tag = "Button_Class";
Button::Button(gpio_num_t p, const char* n){
   pin = p;
   name = n;
   //重置引脚
   gpio_reset_pin(pin);
   //详细配置引脚
   gpio_config_t io_conf = {
   .pin_bit_mask = (1ULL << pin),//选择引脚
   .mode = GPIO_MODE_INPUT,
   .pull_up_en = GPIO_PULLUP_ENABLE,
   .pull_down_en = GPIO_PULLDOWN_DISABLE,//        
   .intr_type = GPIO_INTR_NEGEDGE,//关闭中断
    };
    gpio_config(&io_conf);

}
