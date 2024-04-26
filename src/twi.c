#include "twi.h"

const char *TAG = "I2C";
uint8_t buff[2];
uint8_t cfgBuff[12];
i2c_master_bus_handle_t bus_handle;
i2c_master_dev_handle_t dev_handle;
uint8_t gnTmReg = 0x52;
#define OSR 0x00
#define CREG1 0x06
#define CREG2 0x07
#define CREG3 0x08
#define BREAK 0x09
void twi_init(void)
{
    ESP_LOGI(TAG, "Initialization");
    i2c_master_bus_config_t cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = FLICKERMETER_SCL,
        .sda_io_num = FLICKERMETER_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
        .trans_queue_depth = 10};
    ESP_ERROR_CHECK(i2c_new_master_bus(&cfg, &bus_handle));
    ESP_LOGI(TAG, "Master inicialized");
    ESP_LOGI(TAG, "AS73211 device init");
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = AS73211_ADDRESS,
        .scl_speed_hz = 800000,
    };
    // i2c_master_dev_handle_t dev_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
    ESP_LOGI(TAG, "AS73211 device inicialized");
}
void twi_defaults()
{
    twi_writeReg(CREG1, gnTmReg);
    twi_writeReg(CREG2, 0x00);
    twi_writeReg(CREG3, 0x03);
    twi_writeReg(BREAK, 0x00);
    ESP_LOGI(TAG, "Default states written to device");
    i2c_master_receive(dev_handle, cfgBuff, 12, 100);
    ESP_LOG_BUFFER_HEX("I2C", cfgBuff, 12);
}
void twi_time(uint8_t time)
{
    twi_writeReg(OSR,0x02);
    gnTmReg = (gnTmReg & 0xF0) | (time & 0x0F);
    twi_writeReg(CREG1, gnTmReg);
    twi_writeReg(OSR,0x03);
}
void twi_gain(uint8_t gain)
{
    twi_writeReg(OSR,0x02);
    gnTmReg = (gnTmReg & 0x0F) | (gain & 0xF0);
    twi_writeReg(CREG1, gnTmReg);
    twi_writeReg(OSR,0x03);
}
void twi_start()
{
    twi_writeReg(0x00, 0x03);
    ESP_LOGI(TAG, "Measurement mode Active");
}
void twi_run()
{
    twi_writeReg(0x00, 0x80);
    ESP_LOGI(TAG, "Continuos Measurement");
}
void IRAM_ATTR twi_stop()
{
    twi_writeReg(0x00, 0x00);
    ESP_DRAM_LOGI(TAG, "Stop Measurement");
}
esp_err_t twi_writeReg(uint8_t reg, uint8_t val)
{
    buff[0] = reg;
    buff[1] = val;
    return i2c_master_transmit(dev_handle, buff, 2, 100);
}
void twi_register_callback(const i2c_master_event_callbacks_t *callback)
{
    i2c_master_register_event_callbacks(dev_handle, callback, NULL);
    ESP_LOGI(TAG, "Callback Registered");
}
void twi_unregister_callback(const i2c_master_event_callbacks_t *callback)
{
    i2c_master_register_event_callbacks(dev_handle, NULL, NULL);
}
void IRAM_ATTR twi_read(uint8_t *buffer)
{
    i2c_master_receive(dev_handle, buffer, 6, -1);
}
esp_err_t twi_selftest()
{
    return i2c_master_probe(bus_handle, AS73211_ADDRESS, 1000);
}