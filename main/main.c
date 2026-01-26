#include <stdio.h>
#include <stdint.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PIN_MISO 19 //data from radio to esp
#define PIN_MOSI 23 //data from esp to radio
#define PIN_CLK 18 //clock
#define PIN_CSN 5
#define PIN_CE 4

spi_bus_config_t bus_configuration() {
    printf("Inicializacia busov\n");

    //bus inicialization
    spi_bus_config_t config= {
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
    return config;
}

void app_main(void)
{
    
    spi_bus_config_t bus_cfg = bus_configuration();

    esp_err_t status_message = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);

    if (status_message == ESP_OK) {
        printf("[SUCCESS] SPI Bus is active on pins 18, 19, 23.\n");

        spi_device_interface_config_t dev_config = {
            .mode = 0,                     //NRF24 works in Mode 0 (Clock Low, Phase 1 edge)
            .clock_speed_hz = 4000000,     //4 MHz (max is 10MHz)
            .spics_io_num = PIN_CSN,       //csn pin
            .queue_size = 7,               
        };

        spi_device_handle_t radio; //inititalization nrf card
        esp_err_t status_message_nrf = spi_bus_add_device(SPI2_HOST, &dev_config, &radio);

        if (status_message_nrf == ESP_OK) {
            printf("NRF24 Device successfully added to the bus!\n");
        } else {
            printf("Failed to add NRF24 device. Error: %s\n", esp_err_to_name(status_message_nrf));
        }
    } else {
        printf("[FAIL] Could not init SPI. Error code: %d\n", status_message);
    }

    //ce pin as output
    gpio_set_direction(PIN_CE, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_CE, 0); // Start with radio disabled (Low)

    //continuous loop to stop the esp from sleeping
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Sleep for 1 second
    }
}