#pragma once

#include <stdint.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"

// register definitions
#define NRF_REG_CONFIG      0x00
#define NRF_REG_EN_AA       0x01
#define NRF_REG_SETUP_RETR  0x04
#define NRF_REG_RF_CH       0x05
#define NRF_REG_RF_SETUP    0x06
#define NRF_REG_RPD         0x09

// command definitions
#define NRF_CMD_R_REGISTER  0x00
#define NRF_CMD_W_REGISTER  0x20

// struct that holds radio
typedef struct {
    // bus configuration
    spi_host_device_t host;
    spi_device_handle_t spi;
    
    int miso_pin;
    int mosi_pin;
    int clk_pin;

    // device configuration
    int csn_pin;
    int ce_pin;

    // data and state
    uint16_t hit_counter[128];
    const char* antenna_name;
} nrf24_t;

typedef struct {
    spi_host_device_t host;
    int miso;
    int mosi;
    int sclk;
    int csn;
    int ce;
} nrf_config_t;

// --- Public API Functions ---

// inicializes device and adds it to SPI bus
esp_err_t nrf_attach(spi_host_device_t host, gpio_num_t csn_pin, gpio_num_t ce_pin, nrf24_t *out);

// disables enhanced shockburst and sets data rate to 1Mbps
void nrf_init(nrf24_t *nrf);

// writes value to register
void write_nrf_register(nrf24_t *nrf, uint8_t reg_addr, uint8_t value);

// reads value from register
uint8_t read_nrf_register(nrf24_t *nrf, uint8_t reg_addr);

// loops through the whole bluetooth spectrum and detects hits on channels
void nrf_scan_band(nrf24_t *nrf, uint16_t *hit_counter,uint8_t start_channel, size_t len);

// build radio
esp_err_t setup_nrf_interface(nrf24_t *radio, nrf_config_t config);