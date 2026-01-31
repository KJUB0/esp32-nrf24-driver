#pragma once

#include <stdint.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"

// Register definitions
#define NRF_REG_CONFIG      0x00
#define NRF_REG_EN_AA       0x01
#define NRF_REG_RF_CH       0x05
#define NRF_REG_RF_SETUP    0x06
#define NRF_REG_RPD         0x09

// Command definitions
#define NRF_CMD_R_REGISTER  0x00
#define NRF_CMD_W_REGISTER  0x20

// struct that holds radio
typedef struct {
    spi_device_handle_t spi;
    gpio_num_t ce_pin;
    gpio_num_t csn_pin;
} nrf24_t;    

// --- Public API Functions ---

// inicializes device and adds it to SPI bus
esp_err_t nrf_attach(spi_host_device_t host, gpio_num_t csn_pin, gpio_num_t ce_pin, nrf24_t *out);

// disables enhanced shockburst and sets data rate to 1Mbps
void nrf_init(nrf24_t *nrf);

// Writes value to register
void write_nrf_register(nrf24_t *nrf, uint8_t reg_addr, uint8_t value);

// reads value from register
uint8_t read_nrf_register(nrf24_t *nrf, uint8_t reg_addr);