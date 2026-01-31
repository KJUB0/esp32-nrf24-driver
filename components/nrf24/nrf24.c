#include "nrf24.h"
#include <stdio.h>
#include <string.h>

/**
 * basic chip configuration
 * * Describes how the esp talks to  over already initialized connections
 */
static spi_device_interface_config_t device_configuration(gpio_num_t csn)
{
    //configure the device
    return (spi_device_interface_config_t){
        .mode = 0,                     //NRF24 works in Mode 0 (Clock Low, Phase 1 edge)
        .clock_speed_hz = 4000000,     //4 MHz (max is 10MHz)
        .spics_io_num = csn,           //csn pin
        .queue_size = 7,               
    };
}

/**
 * Performs a single SPI transaction:
 * byte 0: register command + register address
 * byte 1: dummy byte to keep the communication flowing
 * * @return the value stored in the requested register
 */
uint8_t read_nrf_register(nrf24_t *nrf, uint8_t reg_addr) 
{
    spi_transaction_t transaction_container;

    //clean the struct
    transaction_container = (spi_transaction_t){0};

    transaction_container.length = 16; //16 bits (8 command + 8 message)
    transaction_container.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA;

    transaction_container.tx_data[0] = 0x00 | reg_addr;
    transaction_container.tx_data[1] = 0xFF;

    //send the message with the handle (from struct)
    esp_err_t messsage = spi_device_transmit(nrf->spi, &transaction_container);

    if (messsage != ESP_OK){
        printf("Failure during transmission of a message.");
        return 0;
    }
    return transaction_container.rx_data[1];
}

/**
 * Writes a single byte to a specific nrf register
 * * @param payload that we send to the nrf chip
 */
void write_nrf_register(nrf24_t *nrf, uint8_t register_address, uint8_t value)
{
    spi_transaction_t transaction_container = {0};

    transaction_container.length = 16;
    transaction_container.tx_data[0] = 0x20 | (register_address & 0x1F);
    transaction_container.tx_data[1] = value;
    transaction_container.flags =  SPI_TRANS_USE_TXDATA;

    spi_device_transmit(nrf->spi, &transaction_container);
}

esp_err_t nrf_attach(spi_host_device_t host, gpio_num_t csn_pin, gpio_num_t ce_pin, nrf24_t *out)
{
    // Configure CE pin
    gpio_reset_pin(ce_pin);
    gpio_set_direction(ce_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(ce_pin, 0);

    // Configure SPI Device - POZOR: Tu musíme poslať CSN pin, nie CE pin!
    spi_device_interface_config_t cfg = device_configuration(csn_pin);
    
    // Add device to bus
    esp_err_t err = spi_bus_add_device(host, &cfg, &out->spi);
    if (err != ESP_OK) return err;

    out->ce_pin = ce_pin;
    out->csn_pin = csn_pin;
    return ESP_OK;
}

/**
 * disables enhanced shockburst and sets data rate to 1Mbps
 * * @param nrf structure with components
 */
void nrf_init(nrf24_t *nrf)
{
    uint8_t address = 0x01; //address for shockburst register
    uint8_t value = 0x00; //we disable shockburst due to us only listening not sending anything
    write_nrf_register(nrf, address, value);

    // 0000 0110 
    // 3rd bit is 1mbps
    // 2:1 is 0dbm output power
    value = 0x0E; 
    address = 0x06; // RF_SETUP register
    write_nrf_register(nrf, address, value);
}