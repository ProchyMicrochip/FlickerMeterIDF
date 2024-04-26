#pragma once
//#ifndef __TWI_LIB__
//#define __TWI_LIB__
    #include <stdint.h>
    #include "esp_log.h"
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "sdkconfig.h"
    #include "esp_system.h"
    #include "soc/rtc_cntl_reg.h"
    #include <driver/i2c_master.h>
    #include "pins.h"
    //i2c_master_bus_handle_t bus_handle;
    void twi_init();
    esp_err_t twi_selftest();
    void twi_register_callback(const i2c_master_event_callbacks_t* callback);
    void twi_unregister_callback(const i2c_master_event_callbacks_t* callback);
    void IRAM_ATTR twi_read(uint8_t* buffer);
    void IRAM_ATTR twi_stop();
    esp_err_t twi_writeReg(uint8_t reg, uint8_t val);
    void twi_defaults();
    void twi_start();
    void twi_run();
    void twi_time(uint8_t time);
    void twi_gain(uint8_t gain);
//#endif