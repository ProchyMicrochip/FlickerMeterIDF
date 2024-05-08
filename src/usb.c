#include "usb.h"
#include "twi.h"
static const char *TAG = "example";
static uint8_t buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE + 1];
static uint8_t twiBuff[64];
static uint8_t sendBuff[12];
uint32_t count = 0;
uint32_t samples = 16000;
uint8_t databuffer[240000];
uint32_t write_pointer;
uint32_t read_pointer;
uint16_t checksum;
int64_t start = 0;
int64_t end = 0;
bool measure = false;
#define cfg_write(text) tinyusb_cdcacm_write_queue(CFG_CDC, text, strlen((const char *)text))
#define data_write(text) tinyusb_cdcacm_write_queue(DATA_CDC, text, strlen((const char *)text))
#define USBCMP(textA, sizeA, textB, sizeB) sizeA != sizeB ? false : (memcmp(textA, textB, sizeA) == 0 ? true : false)
#define USBCMPPRT(textA, sizeA, textB, sizeB, compareSize) sizeA != sizeB ? false : (memcmp(textA, textB, compareSize) == 0 ? true : false)
static void IRAM_ATTR gpio_interrupt_handler(void *args)
{
    // ESP_DRAM_LOGI("GPIO","Intr %i",count);
    if (count < samples)
    {
        
        if (measure == false)
            return;
        start = esp_timer_get_time();
        twi_read(twiBuff);
        memcpy(&sendBuff, &count, 4);
        memcpy(&sendBuff[4], twiBuff, 6);
        checksum = *(uint16_t *)&sendBuff[0] ^ *(uint16_t *)&sendBuff[2] ^ *(uint16_t *)&sendBuff[4] ^ *(uint16_t *)&sendBuff[6] ^ *(uint16_t *)&sendBuff[8];
        memcpy(&sendBuff[10], &checksum,2);
        memcpy(&databuffer[write_pointer],sendBuff,12);
        write_pointer += 12;
        if(write_pointer == 240000) write_pointer = 0;
        count++;
        end = esp_timer_get_time();
    }
    else
    {
        twi_stop();
        count = 0;
        measure = false;
        //ESP_LOGI("TWI", "execution took %i us",(int)(end-start));
        cfg_write((uint8_t *)"Measurement ended\n");
        tinyusb_cdcacm_write_flush(CFG_CDC, 0);
        //tinyusb_cdcacm_write_flush(DATA_CDC, 0);
    }
}
bool IRAM_ATTR onTwiRecieve(i2c_master_dev_handle_t i2c_dev, const i2c_master_event_data_t *evt_data, void *arg)
{
    // ESP_DRAM_LOGI("I2C","Callback arrived");
    if (measure == false)
        return false;
    memcpy(&sendBuff, &count, 4);
    memcpy(&sendBuff[4], twiBuff, 6);
    checksum = *(uint16_t *)&sendBuff[0] ^ *(uint16_t *)&sendBuff[2] ^ *(uint16_t *)&sendBuff[4] ^ *(uint16_t *)&sendBuff[6] ^ *(uint16_t *)&sendBuff[8];
    memcpy(&sendBuff[10], &checksum,2);
    memcpy(&databuffer[write_pointer],sendBuff,12);
    write_pointer += 12;
    if(write_pointer == 240000) write_pointer = 0;
    //tinyusb_cdcacm_write_queue(DATA_CDC, sendBuff, 10);
    //if((count & 0x000000FF) == 0x000000FF) tinyusb_cdcacm_write_flush(DATA_CDC, 0);
    return true;
}
const i2c_master_event_callbacks_t callbacks = {
    .on_trans_done = &onTwiRecieve,
};
void started(void *pvParameter){
    cfg_write((uint8_t *)"Sensor started\n");
    tinyusb_cdcacm_write_flush(CFG_CDC, 0);
    vTaskDelete(NULL);
}
void start_sensor(void *pvParameter)
{
    //twi_register_callback(&callbacks);
    twi_defaults();
    ESP_LOGI("TWI","Defaults written");
    twi_start();
    ESP_LOGI("TWI","AS73211 started");
    gpio_set_direction(FLICKERMETER_RDY, GPIO_MODE_INPUT);
    gpio_pulldown_dis(FLICKERMETER_RDY);
    gpio_pullup_dis(FLICKERMETER_RDY);
    gpio_set_intr_type(FLICKERMETER_RDY, GPIO_INTR_POSEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(FLICKERMETER_RDY, gpio_interrupt_handler, (void *)FLICKERMETER_RDY);
    ESP_LOGI("GPIO","Interrupt attached");
    xTaskCreatePinnedToCore(&started, "started", 1024, NULL, 4, NULL, 1);
    vTaskDelete(NULL);
    //twi_run();
}




void tinyusb_data_rx_callback(int itf, cdcacm_event_t *event)
{
    size_t rx_size = 0;
    ESP_LOGI("DATA", "Data interface recieved data");
    esp_err_t ret = tinyusb_cdcacm_read(itf, buf, CONFIG_TINYUSB_CDC_RX_BUFSIZE, &rx_size);
    if (ret != ESP_OK)
        return;
    if (USBCMP(buf, rx_size, "info\n", 5))
    {
        data_write((uint8_t *)"Data interface of Flickermeter\n");
    }
    /* write back */
    // tinyusb_cdcacm_write_queue(itf, buf, rx_size);
    tinyusb_cdcacm_write_flush(itf, 0);
}
void tinyusb_cfg_rx_callback(int itf, cdcacm_event_t *event)
{
    /* initialization */
    size_t rx_size = 0;
    ESP_LOGI("CONFIG", "Cfg interface recieved data");
    /* read */
    esp_err_t ret = tinyusb_cdcacm_read(itf, buf, CONFIG_TINYUSB_CDC_RX_BUFSIZE, &rx_size);
    if(measure == true){
        cfg_write((uint8_t *)"Busy\n");
        tinyusb_cdcacm_write_flush(itf, 0);
        return;
    }
    if (ret != ESP_OK)
        return;
    if (USBCMP(buf, rx_size, "init\n", 5))
    {
        twi_init();
        cfg_write((uint8_t *)"Device Inicialization\n");
    }
    else if (USBCMP(buf, rx_size, "info\n", 5))
    {
        cfg_write((uint8_t *)"Config interface of Flickermeter\n");
    }
    else if (USBCMP(buf, rx_size, "run\n", 4))
    {
        write_pointer = 0;
        read_pointer = 0;
        cfg_write((uint8_t *)"Measurement started\n");
        twi_run();
        measure = true;
    }
    else if(USBCMPPRT(buf, rx_size, "Gain 0x00\n", 10 , 8)){
        twi_gain(buf[8] - '0');
        char snd[32];
        sprintf(snd,"Gain Updated 0x%02X\n",buf[8] - '0');
        cfg_write((uint8_t *)snd);
    }
    else if(USBCMPPRT(buf, rx_size, "Time 0x00\n", 10 , 8)){
        twi_time(buf[8] - '0');
        char snd[32];
        sprintf(snd,"Time Updated 0x%02X\n",buf[8] - '0');
        cfg_write((uint8_t *)snd);
    }
    else if(USBCMPPRT(buf, rx_size, "Sample 0xFFFFFFFF\n", 18, 9)){
        samples =((buf[9] - '0') << 28) | ((buf[10] - '0') << 24) | ((buf[11] - '0') << 20) | ((buf[12] - '0') << 16) | ((buf[13] - '0') << 12) | ((buf[14] - '0') << 8) | ((buf[15] - '0') << 4) | (buf[16] - '0');
        char snd[32];
        sprintf(snd,"Sample Updated 0x%08X\n",(uint)samples);
        //Add additional samples to pad Real data
        //samples += 1000;
        cfg_write((uint8_t *)snd);
    }
    else if (USBCMP(buf, rx_size, "self\n", 5))
    {
        ESP_LOGI("Test", "measurement took %li us", (long int)(end-start));
        esp_err_t as73211 = twi_selftest();
        if (as73211 == ESP_OK)
        {
            cfg_write((uint8_t *)"Self test OK\n");
        }
        else
        {
            cfg_write((uint8_t *)"Self test FAIL\n");
        }
    }
    else if (USBCMP(buf, rx_size, "start\n", 6))
    {
        xTaskCreatePinnedToCore(&start_sensor,"INTERRUPT", 3000, NULL, 4, NULL, 0);
        
    }
    tinyusb_cdcacm_write_flush(itf, 0);
}

void cdcTask(void *pvParameter){
    while (true)
    {
        if(write_pointer != read_pointer){
            if(write_pointer < read_pointer){
                if(write_pointer - read_pointer + 240000 >= 12000){
                    tinyusb_cdcacm_write_queue(DATA_CDC,&databuffer[read_pointer],12000);
                    //tinyusb_cdcacm_write_flush(DATA_CDC,0);
                    read_pointer += 12000;
                    if(read_pointer == 240000) read_pointer = 0;
                }
            }else{
                if(write_pointer - read_pointer >= 12000){
                    tinyusb_cdcacm_write_queue(DATA_CDC,&databuffer[read_pointer],12000);
                    //tinyusb_cdcacm_write_flush(DATA_CDC,0);
                    read_pointer += 12000;
                    if(read_pointer == 240000) read_pointer = 0;
                }
            }
            
        }
        vTaskDelay(portTICK_PERIOD_MS);
       //vTaskDelay(5*portTICK_PERIOD_MS);
    }
    /*uint32_t dataSize = 1200000;
    int64_t start;
    int64_t end;
    int transferSize = 12000;
    uint32_t measurements = dataSize/transferSize;
    ESP_LOGI("timer", "starting meassurement");
    start = esp_timer_get_time();
    for (size_t i = 0; i < measurements; i++)
    {
        tinyusb_cdcacm_write_queue(DATA_CDC,&databuffer[read_pointer],transferSize);
        //tinyusb_cdcacm_write_flush(DATA_CDC,3000);
    }
    tinyusb_cdcacm_write_flush(DATA_CDC,3000);
    end = esp_timer_get_time();
    ESP_LOGI("timer", "Transfer with data size %i was completed with Throughput %f KB/s",transferSize, (dataSize*1000)/((float)(end-start)));
    vTaskDelay(200 * portTICK_PERIOD_MS);
    ESP_LOGI("timer", "starting meassurement");
    transferSize = 6000;
    measurements = dataSize/transferSize;
    start = esp_timer_get_time();
    for (size_t i = 0; i < measurements; i++)
    {
        tinyusb_cdcacm_write_queue(DATA_CDC,&databuffer[read_pointer],transferSize);
        //tinyusb_cdcacm_write_flush(DATA_CDC,3000);
    }
    tinyusb_cdcacm_write_flush(DATA_CDC,3000);
    end = esp_timer_get_time();
    ESP_LOGI("timer", "Transfer with data size %i was completed with Throughput %f KB/s",transferSize, (dataSize*1000)/((float)(end-start)));
    vTaskDelay(200 * portTICK_PERIOD_MS);
    ESP_LOGI("timer", "starting meassurement");
    transferSize = 3000;
    measurements = dataSize/transferSize;
    start = esp_timer_get_time();
    for (size_t i = 0; i < measurements; i++)
    {
        tinyusb_cdcacm_write_queue(DATA_CDC,&databuffer[read_pointer],transferSize);
        //tinyusb_cdcacm_write_flush(DATA_CDC,3000);
    }
    tinyusb_cdcacm_write_flush(DATA_CDC,3000);
    end = esp_timer_get_time();
    ESP_LOGI("timer", "Transfer with data size %i was completed with Throughput %f KB/s",transferSize, (dataSize*1000)/((float)(end-start)));
    vTaskDelay(2000 * portTICK_PERIOD_MS);
    ESP_LOGI("timer", "starting meassurement");
    transferSize = 1500;
    measurements = dataSize/transferSize;
    start = esp_timer_get_time();
    for (size_t i = 0; i < measurements; i++)
    {
        tinyusb_cdcacm_write_queue(DATA_CDC,&databuffer[read_pointer],transferSize);
        //tinyusb_cdcacm_write_flush(DATA_CDC,3000);
    }
    tinyusb_cdcacm_write_flush(DATA_CDC,3000);
    end = esp_timer_get_time();
    ESP_LOGI("timer", "Transfer with data size %i was completed with Throughput %f KB/s",transferSize, (dataSize*1000)/((float)(end-start)));
    vTaskDelay(200 * portTICK_PERIOD_MS);
    ESP_LOGI("timer", "starting meassurement");
    transferSize = 500;
    measurements = dataSize/transferSize;
    start = esp_timer_get_time();
    for (size_t i = 0; i < measurements; i++)
    {
        tinyusb_cdcacm_write_queue(DATA_CDC,&databuffer[read_pointer],transferSize);
        //tinyusb_cdcacm_write_flush(DATA_CDC,3000);
    }
    tinyusb_cdcacm_write_flush(DATA_CDC,3000);
    end = esp_timer_get_time();
    ESP_LOGI("timer", "Transfet with data size %i was completed with Throughput %f KB/s",transferSize, (dataSize*1000)/((float)(end-start)));
    */
    vTaskDelete(NULL);
}

void usb_init(void)
{
    ESP_LOGI(TAG, "USB initialization");
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = NULL,
        .external_phy = false,
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    tinyusb_config_cdcacm_t acm_cfg = {
        .usb_dev = TINYUSB_USBDEV_0,
        .cdc_port = TINYUSB_CDC_ACM_0,
        .rx_unread_buf_sz = 64,
        .callback_rx = &tinyusb_data_rx_callback, // the first way to register a callback
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = NULL,
        .callback_line_coding_changed = NULL};
    tinyusb_config_cdcacm_t acm_cfg2 = {
        .usb_dev = TINYUSB_USBDEV_0,
        .cdc_port = TINYUSB_CDC_ACM_1,
        .rx_unread_buf_sz = 64,
        .callback_rx = &tinyusb_cfg_rx_callback, // the first way to register a callback
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = NULL,
        .callback_line_coding_changed = NULL};

    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));
    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg2));

    ESP_LOGI(TAG, "USB initialization DONE");
    xTaskCreatePinnedToCore(&cdcTask, "cdc_task", 2048, NULL, 4, NULL, 1);
}