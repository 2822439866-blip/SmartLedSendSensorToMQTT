#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "MPU6050.h"
// 配置参数
#define I2C_MASTER_SCL_IO           22          // SCL 引脚
#define I2C_MASTER_SDA_IO           21          // SDA 引脚
#define I2C_MASTER_NUM              0           // I2C 端口号
#define I2C_MASTER_FREQ_HZ          400000      // 400kHz 快速模式

// MPU6050 寄存器地址
#define MPU6050_DEV_ADDR            0x68        // MPU6050 I2C  slave 地址
#define MPU6050_REG_WHO_AM_I        0x75        // WHO_AM_I 寄存器 (默认返回 0x68)
#define MPU6050_REG_PWR_MGMT_1      0x6B        // 电源管理寄存器
#define MPU6050_REG_ACCEL_XOUT_H    0x3B        // 加速度计 X 轴高位地址

#define SYS_STATE_BIT_WIFI_CONNECTED BIT0
#define SYS_STATE_BIT_MQTT_CONNECTED BIT1
#define SYS_STATE_BIT_MOVING BIT2
#define SYS_STATE_BIT_ALL_NORMAL (SYS_STATE_BIT_WIFI_CONNECTED | SYS_STATE_BIT_MQTT_CONNECTED | SYS_STATE_BIT_MOVING)

const char* MPU6050::MPU6050_tag = nullptr;
EventGroupHandle_t sys_state_group = nullptr;
MPU6050::MPU6050(EventGroupHandle_t sys_state_group)
{
    MPU6050::sys_state_group = sys_state_group;
    i2c_master_dev_handle_t dev_handle;
    // 1. 配置 I2C 总线
    i2c_master_bus_config_t i2c_mst_config = {};
    i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;//I2C 外设的时钟源选择
    i2c_mst_config.i2c_port = I2C_MASTER_NUM;//指定使用哪一个 I2C 硬件端口
    i2c_mst_config.scl_io_num = (gpio_num_t)I2C_MASTER_SCL_IO;
    i2c_mst_config.sda_io_num = (gpio_num_t)I2C_MASTER_SDA_IO;
    i2c_mst_config.glitch_ignore_cnt = 7;
    i2c_mst_config.flags.enable_internal_pullup = true;
    
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    // 2. 将 MPU6050 挂载到 I2C 总线上
    i2c_device_config_t dev_cfg = {};
    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = MPU6050_DEV_ADDR;
    dev_cfg.scl_speed_hz = I2C_MASTER_FREQ_HZ;
    
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    // 3. 唤醒 MPU6050 (默认上电是睡眠模式，往 0x6B 写入 0x00)
    uint8_t wake_cmd[2] = {MPU6050_REG_PWR_MGMT_1, 0x00};
    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, wake_cmd, sizeof(wake_cmd), -1));
    this->dev_handle = dev_handle;
    MPU6050_tag = "MPU6050";
    this->datalist_init();
}

void MPU6050::datalist_init()
{
    for(int i=0;i<=9;i++){
        uint8_t datalist[14];
        uint8_t reg_addr = MPU6050_REG_ACCEL_XOUT_H;
        esp_err_t ret = i2c_master_transmit_receive(dev_handle, &reg_addr, 1, datalist, 14, -1);
        if(ret == ESP_OK){
        acc_x[i] = (int64_t)(datalist[0] << 8) | datalist[1];
        acc_y[i] = (int64_t)(datalist[2] << 8) | datalist[3];
        acc_z[i] = (int64_t)(datalist[4] << 8) | datalist[5];
        gyro_x[i] = (int64_t)(datalist[8] << 8) | datalist[9];
        gyro_y[i] = (int64_t)(datalist[10] << 8) | datalist[11];
        gyro_z[i] = (int64_t)(datalist[12] << 8) | datalist[13];
        }else{ESP_LOGI(MPU6050_tag,"读取数据错误");}
    }
}

void MPU6050::put_data_in_datalist()
{   
    uint8_t datalist[14];
    uint8_t reg_addr = MPU6050_REG_ACCEL_XOUT_H;    
    // 一次性读取 14 字节 加速度3轴*2  温度*2  陀螺仪3轴*2）
    esp_err_t ret = i2c_master_transmit_receive(dev_handle, &reg_addr, 1, datalist, 14, -1);
    if(ret == ESP_OK){
        acc_x[this->cont] = (int64_t)(datalist[0] << 8) | datalist[1];
        acc_y[this->cont] = (int64_t)(datalist[2] << 8) | datalist[3];
        acc_z[this->cont] = (int64_t)(datalist[4] << 8) | datalist[5];
            
        gyro_x[this->cont] = (int64_t)(datalist[8] << 8) | datalist[9];
        gyro_y[this->cont] = (int64_t)(datalist[10] << 8) | datalist[11];
        gyro_z[this->cont] = (int64_t)(datalist[12] << 8) | datalist[13];

        this->cont++;
        this->cont = (this->cont)%10;

        //ESP_LOGI(MPU6050_tag, "ACC: X=%d, Y=%d, Z=%d | GYRO: X=%d, Y=%d, Z=%d", 
             //           this->acc_x[this->cont], this->acc_y[this->cont], this->acc_z[this->cont], 
           //             this->gyro_x[this->cont], this->gyro_y[this->cont], this->gyro_z[this->cont]);
    }else{ESP_LOGI(MPU6050_tag,"读取数据错误");}
    
}

void MPU6050::calculate_ave_and_s()
{   
    long sum_acc_x = 0;
    long sum_acc_y = 0;
    long sum_acc_z = 0;

    long sum_gyro_x = 0;
    long sum_gyro_y = 0;
    long sum_gyro_z = 0;
    for(int i=0;i<=9;i++){
        sum_acc_x += acc_x[i];
        sum_acc_y += acc_y[i];
        sum_acc_z += acc_z[i];

        sum_gyro_x += gyro_x[i];
        sum_gyro_y += gyro_y[i];
        sum_gyro_z += gyro_z[i];
    }
    ave_acc_x = sum_acc_x/10.0f;
    ave_acc_y = sum_acc_y/10.0f;
    ave_acc_z = sum_acc_z/10.0f;
    ave_gyro_x = sum_gyro_x/10.0f;
    ave_gyro_y = sum_gyro_y/10.0f;
    ave_gyro_z = sum_gyro_z/10.0f;

    long sum_acc_x_s =0;
    long sum_acc_y_s =0;
    long sum_acc_z_s =0;
    long sum_gyro_x_s =0;
    long sum_gyro_y_s =0;
    long sum_gyro_z_s =0;
    for(int i=0;i<=9;i++){
        sum_acc_x_s += (acc_x[i] - ave_acc_x)*(acc_x[i] - ave_acc_x);
        sum_acc_y_s += (acc_y[i] - ave_acc_y)*(acc_y[i] - ave_acc_y);
        sum_acc_z_s += (acc_z[i] - ave_acc_z)*(acc_z[i] - ave_acc_z);

        sum_gyro_x_s += (gyro_x[i] - ave_gyro_x)*(gyro_x[i] - ave_gyro_x);
        sum_gyro_y_s += (gyro_y[i] - ave_gyro_y)*(gyro_y[i] - ave_gyro_y);
        sum_gyro_z_s += (gyro_z[i] - ave_gyro_z)*(gyro_z[i] - ave_gyro_z);
    }
    s_acc_x = sum_acc_x_s/10.0f;
    s_acc_y = sum_acc_y_s/10.0f;
    s_acc_z = sum_acc_z_s/10.0f;
    s_gyro_x = sum_gyro_x_s/10.0f;
    s_gyro_y = sum_gyro_y_s/10.0f;
    s_gyro_z = sum_gyro_z_s/10.0f;
    //ESP_LOGI(MPU6050_tag,"s_ACC: X=%f, Y=%f, Z=%f | s_GYRO: X=%f, Y=%f, Z=%f",
     //                   this->s_acc_x,this->s_acc_y,this->s_acc_z,this->s_gyro_x,this->s_gyro_y,this->s_gyro_z);
}


void MPU6050::change_sys_state()
{
    //只要有一个方差超过给定值就标记为移动，否则标记为静止
    //移动的话，开启正常 否则低功耗模式
    if((s_acc_x > 150000) or (s_acc_y > 150000) or (s_acc_z > 150000) or (s_gyro_x > 150000) or (s_gyro_y > 150000) or (s_gyro_z > 150000)){
        xEventGroupSetBits(this->sys_state_group,SYS_STATE_BIT_MOVING);
    }else{xEventGroupClearBits(this->sys_state_group,SYS_STATE_BIT_MOVING);}
}

EventBits_t MPU6050::get_sys_state_group()
{
    EventBits_t sys_state_bit = xEventGroupGetBits(this->sys_state_group);
    return sys_state_bit;
}
