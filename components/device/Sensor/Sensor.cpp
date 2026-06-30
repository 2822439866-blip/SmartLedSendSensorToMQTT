#include "Sensor.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
static const char* sensor_tag = "Sensor_Class";
Sensor::Sensor(int gpio, const char* n){
    adc_handle = NULL;
    //通过gpio得到通道和ADC核号
    adc_unit_t unit_id;
    adc_channel_t channel_id;
    adc_oneshot_io_to_channel(gpio,&unit_id,&channel_id);
    this->adc_chan = channel_id;
    this->unit_id = unit_id;
    name = n;
    ESP_LOGW(sensor_tag, "Sensor %s: GPIO %d -> ADC Unit %d Channel %d", name, gpio, unit_id + 1, channel_id);

    adc_oneshot_unit_init_cfg_t adc_config = {};
   adc_config.unit_id = unit_id;//adc_unit_t
   // 创建 ADC 单元的句柄，后续对 ADC 的操作都通过这个句柄来进行
   //必须调用这个函数，句柄才真正创建出来 了，否则 adc_handle 还是 NULL，后续调用 adc_oneshot_config_channel 和 adc_oneshot_read 都会崩溃
   ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_config, &adc_handle));
   this->adc_handle = adc_handle;
   //配置ADC通道
   adc_oneshot_chan_cfg_t chan_config = {
      .atten = ADC_ATTEN_DB_12, // 12dB 衰减
      .bitwidth = ADC_BITWIDTH_DEFAULT, // 默认 12 位宽
   };
   ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle,  adc_chan, &chan_config));
   sensorInit();
}  
int Sensor::readValue(){
   int val;
   ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, adc_chan, &val));
   return val;
}
adc_oneshot_unit_handle_t Sensor::getAdcHandle(){
   return adc_handle;
}
void Sensor::sensorInit(){
   for(int i=0;i<=9;i++){
      sensorList[i] = readValue();
   }
}
int Sensor::getsensorListAve(){
    int sum = 0;
    for(int i=0;i<=9;i++){
        sum += sensorList[i];
    }
    return sum/10;
}

