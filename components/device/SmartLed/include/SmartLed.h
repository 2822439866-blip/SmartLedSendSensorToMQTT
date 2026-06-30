#pragma once
#include<stdio.h>
#include "driver/gpio.h"
class SmartLed{
   private:
      gpio_num_t pin;
      const char* name;
      int brightTime;
      int onbrightTime;   
   public:
      bool state;
      SmartLed(gpio_num_t p, const char* n,int ont,int offt);
      void blink();
      void flash();
      void close();
};