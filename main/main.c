#include <stdio.h>
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PIN_MISO 19 //data from radio to esp
#define PIN_MOSI 23 //data from esp to radio
#define PIN_CLK 18 //clock

void app_main(void)
{
    printf("Inicializacia busov");

    //bus inicialization
    spi_bus_config_t busconfig= {
        .miso_io_num = PIN_MISO,
        .mosi_io_num = PIN_MOSI,
        .sclk_io_num = PIN_CLK,
        
        //inicialization of pins for display to values -1 
        //-1 tells chip that we dont use these ports
        .quadwp_io_num = -1, 
        .quadhd_io_num = -1,

        //limit for transactions
        .max_transfer_sz = 32 //bytes
    };


    esp_err_t status_message = spi_bus_initialize(SPI2_HOST, &busconfig, SPI_DMA_CH_AUTO);

    if (status_message == ESP_OK) {
        printf("[SUCCESS] SPI Bus is active on pins 18, 19, 23.\n");
    } else {
        printf("[FAIL] Could not init SPI. Error code: %d\n", status_message);
    }

    //continuous loop to stop the esp from sleeping
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Sleep for 1 second
    }
}