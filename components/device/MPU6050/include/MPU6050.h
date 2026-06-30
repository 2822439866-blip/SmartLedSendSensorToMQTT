#pragma once
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/i2c_master.h"
#include "freertos/event_groups.h"
class MPU6050{
    private:
        i2c_master_dev_handle_t dev_handle;
        EventGroupHandle_t sys_state_group;
        int cont = 0;
        int16_t acc_x[10] = {0};
        int16_t acc_y[10] = {0};
        int16_t acc_z[10] = {0};
        int16_t gyro_x[10]  = {0};
        int16_t gyro_y[10] = {0};
        int16_t gyro_z[10] ={0};

        float ave_acc_x = 0;
        float ave_acc_y = 0;
        float ave_acc_z = 0;
        float s_acc_x = 0;
        float s_acc_y = 0;
        float s_acc_z = 0;

        float ave_gyro_x = 0;
        float ave_gyro_y = 0;
        float ave_gyro_z = 0;
        float s_gyro_x = 0;
        float s_gyro_y = 0;
        float s_gyro_z = 0;
 
    public:
        bool last_sys_state_is_static = false;
        bool curr_sys_state_is_static = false;
        static const char* MPU6050_tag;
        MPU6050(EventGroupHandle_t sys_state_group);
        void datalist_init();
        void put_data_in_datalist();
        void calculate_ave_and_s();
        void change_sys_state();
        EventBits_t get_sys_state_group();

        
};