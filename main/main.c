#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "nrf24.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"

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
    // try to read register
    // by default the register should be at 0x00
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

/**
 * function that prints the data for poython script for monitoring
 * 
 * @param  Number of hits detected on the channel
 * @param lenght of bluetooth band
 * @param if the antenna is left or right
 */
void send_csv_data(uint16_t *hit_counter, size_t len, const char* label) 
{
    // send label that the python script expects
    printf("DATA_%s:", label); 
    
    for (int i = 0; i < len; i++) {
        printf("%d", hit_counter[i]);
        if (i < len - 1) {
            printf(",");
        }
    }
    printf("\n"); // end of line that triggers readline in python
}

void app_main(void) 
{
    nrf24_t radio_left;
    nrf24_t radio_right;

    // first radio configuration spi2
    nrf_config_t config_left = {
        .host = SPI2_HOST, 
        .miso = 19, 
        .mosi = 23, 
        .sclk = 18, 
        .csn = 21, 
        .ce = 22
    };

    //  second radio config spi3
    nrf_config_t config_right = {
        .host = SPI3_HOST, 
        .miso = 12, 
        .mosi = 13, 
        .sclk = 14, 
        .csn = 15, 
        .ce = 16
    };

    // initialize both
    printf("Initializing Left Radio...\n");
    esp_err_t ret = setup_nrf_interface(&radio_left, config_left);
    if (ret != ESP_OK) {
        printf("Failed to init Left Radio: %d\n", ret);
        return;
    }

    printf("Initializing Right Radio...\n");
    ret = setup_nrf_interface(&radio_right, config_right);
    if (ret != ESP_OK) {
        printf("Failed to init Right Radio: %d\n", ret);
        return;
    }

    // connections test
    printf("--- Testing Left Radio ---\n");
    test_nrf_connection(&radio_left);
    
    printf("--- Testing Right Radio ---\n");
    test_nrf_connection(&radio_right);

    // band length scanning parameter
    size_t band_len = 40;

    while(1) {
        // Clear previous results
        memset(radio_left.hit_counter, 0, sizeof(radio_left.hit_counter));
        memset(radio_right.hit_counter, 0, sizeof(radio_right.hit_counter));

        // Perform scans
        for (int i = 0; i < 50; i++) {
            nrf_scan_band(&radio_left, radio_left.hit_counter, 0, band_len);
            nrf_scan_band(&radio_right, radio_right.hit_counter, 40, band_len);
            // Give the CPU a tiny break to prevent watchdog issues
            vTaskDelay(1); 
        }

        // SEND DATA TO PYTHON
        send_csv_data(radio_left.hit_counter, band_len, "LEFT");
        send_csv_data(radio_right.hit_counter, band_len, "RIGHT");

        vTaskDelay(pdMS_TO_TICKS(200)); 
    }

    keep_alive_loop();
}