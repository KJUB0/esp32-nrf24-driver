#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "nrf24.h" // Include header file
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PIN_MISO 19 //data from radio to esp
#define PIN_MOSI 23 //data from esp to radio
#define PIN_CLK 18 //clock
#define PIN_CSN 5 //chip select (active low)
#define PIN_CE 4 //chip enable

/**
 * function to assign pins and configure buses
 * * This function prepares the SPI bus configuration structure.
 * It does NOT initialize the bus itself — that is done later
 * by spi_bus_initialize().
 */
static spi_bus_config_t bus_configuration(void) 
{
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

/**
 * Calls a function that assigns pins and then initializes 
 * SPi with them
 */
static esp_err_t init_spi_bus(void)
{
    spi_bus_config_t bus_cfg = bus_configuration(); //
    return spi_bus_initialize(SPI3_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
}

/**
 * infinite loop to prevent the application from exitting
 */
static void keep_alive_loop(void)
{
    //continuous loop to stop the esp from sleeping
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Sleep for 1 second
    }
}

/**
 * checks if we've successfuly communicated with the nrf module
 * reads the register to check for reset values
 * * @param nrf structure with components
 */
static void test_nrf_connection(nrf24_t *radio)
{
    //try to read register
    //by default the register should be at 0x00
    uint8_t config_value = read_nrf_register(radio, 0x00);

    printf("Read CONFIG Register (0x00): 0x%02X\n", config_value);
    
    if (config_value == 0x08 || config_value == 0x0E || config_value == 0x0F || config_value == 0x00) {
        printf("[TEST PASSED] NRF24 is wired correctly and responding!\n");
    } else if (config_value == 0xFF) {
        printf("[TEST FAILED] NRF24 read 0xFF. Check your wiring (MISO/MOSI).\n");
    } else {
        printf("[WARNING] NRF24 responded, but value is unexpected.\n");
    }
}

void print_histogram(uint16_t *hit_counter, size_t len)
{
    uint16_t maximum = 0;
    uint16_t maximum_channel = 0;
    for (uint16_t i = 0; i < len; i++) {
        if (hit_counter[i] > maximum) {
            maximum = hit_counter[i];
            maximum_channel = i;
        }
    }

    for (int i = maximum; i > 0; i--) {
        for (uint16_t j = 0; j < len; j++) {
            if (hit_counter[i] >= i)
                printf("#");
        }
        printf("\n");
    }
}

void app_main(void)
{
    esp_err_t status_message = init_spi_bus();

    if (status_message == ESP_OK) {
        printf("[SUCCESS] SPI Bus is active on pins 18, 19, 23.\n");

        nrf24_t radio; // struct for radio
        
        // Use library function to attach device (replaces local init_nrf_device and init_ce_pin)
        esp_err_t status_message_nrf = nrf_attach(SPI3_HOST, PIN_CSN, PIN_CE, &radio);

        if (status_message_nrf == ESP_OK) {
            printf("NRF24 Device successfully added to the bus!\n");
            
            test_nrf_connection(&radio); // pass pointer to struct
            
            nrf_init(&radio); // call lib init

            while (1) {
                uint16_t hit_counter[127];
                size_t band_len = 126;
                nrf_scan_band(&radio, &hit_counter, band_len);
            }

        } else {
            printf("Failed to add NRF24 device. Error: %s\n",
                   esp_err_to_name(status_message_nrf));
        }
    } else {
        printf("[FAIL] Could not init SPI. Error code: %d\n", status_message);
    }

    // init_ce_pin() removed because nrf_attach handles it now
    keep_alive_loop();
}