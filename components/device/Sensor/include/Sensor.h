#pragma once
#include<stdio.h>
#include "esp_adc/adc_oneshot.h"
class Sensor{
   private:
        adc_oneshot_unit_handle_t  adc_handle;
	     adc_channel_t  adc_chan;
        adc_unit_t unit_id;
        const char* name;

   public:
      Sensor(int gpio, const char* n);
      int readValue();
      adc_oneshot_unit_handle_t getAdcHandle();
      void sensorInit();
      int getsensorListAve();
      int sensorList[10];
};