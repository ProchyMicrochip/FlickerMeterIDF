#pragma once
    #include <stdint.h>
    #include "esp_log.h"
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "tinyusb.h"
    #include "tusb_cdc_acm.h"
    #include "tusb_console.h"
    #include "sdkconfig.h"
    #include "esp_system.h"
    #include "soc/rtc_cntl_reg.h"
    #include <driver/i2c_master.h>
    #include "driver/gpio.h"
    #include <inttypes.h>
    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
    #include "esp_system.h"
    //const i2c_master_event_callbacks_t callbacks;
    void usb_init();
    //bool IRAM_ATTR onTwiRecieve(i2c_master_dev_handle_t i2c_dev, const i2c_master_event_data_t *evt_data, void *arg);
