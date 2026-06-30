#pragma once
#include<stdio.h>
#include "driver/gpio.h"
class Button{
   private:
        gpio_num_t pin;
        const char* name;
        
   public:
          Button(gpio_num_t p, const char* n);
          static const char* button_tag;
};
