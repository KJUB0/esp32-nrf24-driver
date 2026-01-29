#include <stdio.h>
#include <stdint.h>
#include <string.h>
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
 * 
 * This function prepares the SPI bus configuration structure.
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
 * Performs a single SPI transaction:
 *  byte 0: register command + register address
 *  byte 1: dummy byte to keep the communication flowing
 * 
 * @param SPI device handle
 *  
 * @param 5-bit adress of the register to read
 * 
 * @return the value stored in the requested register
 */
uint8_t read_nrf_register(spi_device_handle_t handle, uint8_t reg_addr) 
{
    spi_transaction_t transaction_container;

    //clean the struct
    transaction_container = (spi_transaction_t){0};

    transaction_container.length = 16; //16 bits (8 command + 8 message)
    transaction_container.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA;

    transaction_container.tx_data[0] = 0x00 | reg_addr;
    transaction_container.tx_data[1] = 0xFF;

    //send the message with the handle and with the transmission container
    esp_err_t messsage = spi_device_transmit(handle, &transaction_container);

    if (messsage != ESP_OK){
        printf("Failure during transmission of a message.");
        return 0;
    }
    return transaction_container.rx_data[1];
}

/**
 * basic chip configuration
 * 
 * Describes how the esp talks to nrf over already initialized connections
 */
static spi_device_interface_config_t device_configuration(void)
{
    //configure the device
    spi_device_interface_config_t dev_config = {
        .mode = 0,                     //NRF24 works in Mode 0 (Clock Low, Phase 1 edge)
        .clock_speed_hz = 4000000,     //4 MHz (max is 10MHz)
        .spics_io_num = PIN_CSN,       //csn pin
        .queue_size = 7,               
    };
    return dev_config;
}

/**
 * Writes a single byte to a specific nrf register
 * 
 * @param SPI device handle
 * 
 * @param registeradress to write to
 * 
 * @param payload that we send to the nrf chip
 */
void write_nrf_register(spi_device_handle_t handle, uint8_t register_address, uint8_t value)
{
    spi_transaction_t transaction_container;
    transaction_container = (spi_transaction_t){0};

    transaction_container.length = 16;
    transaction_container.tx_data[0] = 0x20 | register_address;
    transaction_container.tx_data[1] = value;
    transaction_container.flags = SPI_TRANS_USE_TXDATA;

    spi_device_transmit(handle, &transaction_container);
}

/**
 * Calls a function that assigns pins and then initializes 
 * SPi with them
 */
static esp_err_t init_spi_bus(void)
{
    spi_bus_config_t bus_cfg = bus_configuration(); //
    return spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
}

/**
 * nrf device inicialization and attaches the device to the already created SPI bus
 */
static esp_err_t init_nrf_device(spi_device_handle_t *radio)
{
    //configure the device
    spi_device_interface_config_t dev_config = device_configuration();
    return spi_bus_add_device(SPI2_HOST, &dev_config, radio);
}


/**
 * checks if we've successfuly communicated with the nrf module
 * reads the register to check for reset values
 * 
 * @param nrf structure with components
 */
static void test_nrf_connection(spi_device_handle_t radio)
{
    //try to read register
    //by default the register should be at 0x00
    uint8_t config_value = read_nrf_register(radio, 0x00);

    printf("Read CONFIG Register (0x00): 0x%02X\n", config_value);
    
    if (config_value == 0x08 || config_value == 0x0E || config_value == 0x0F) {
        printf("[TEST PASSED] NRF24 is wired correctly and responding!\n");
    } else if (config_value == 0x00 || config_value == 0xFF) {
        printf("[TEST FAILED] NRF24 is silent. Check your wiring (MISO/MOSI).\n");
    } else {
        printf("[WARNING] NRF24 responded, but value is unexpected.\n");
    }
}

/**
 * configures the ce pin used to control the radio state
 * CE low keeps the radio disabled
 */
static void init_ce_pin(void)
{
    //ce pin as output
    gpio_set_direction(PIN_CE, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_CE, 0); // Start with radio disabled (Low)
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

void app_main(void)
{
    esp_err_t status_message = init_spi_bus();

    if (status_message == ESP_OK) {
        printf("[SUCCESS] SPI Bus is active on pins 18, 19, 23.\n");

        spi_device_handle_t radio; //inititalization nrf card
        esp_err_t status_message_nrf = init_nrf_device(&radio);

        if (status_message_nrf == ESP_OK) {
            printf("NRF24 Device successfully added to the bus!\n");
            test_nrf_connection(radio);
        } else {
            printf("Failed to add NRF24 device. Error: %s\n",
                   esp_err_to_name(status_message_nrf));
        }
    } else {
        printf("[FAIL] Could not init SPI. Error code: %d\n", status_message);
    }

    init_ce_pin();
    keep_alive_loop();
}