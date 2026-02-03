#include "nrf24.h"
#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_rom_sys.h"

// --- Internal Helper Functions ---

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
 * function to assign pins and configure buses
 */
static spi_bus_config_t bus_configuration(int miso, int mosi, int clk) 
{
    printf("Inicializacia busov\n");

    //bus inicialization
    spi_bus_config_t config= {
        .miso_io_num = miso,
        .mosi_io_num = mosi,
        .sclk_io_num = clk,
        .quadwp_io_num = -1, 
        .quadhd_io_num = -1,
        .max_transfer_sz = 32 //bytes
    };
    return config;
}

/**
 * Calls a function that assigns pins and then initializes SPi with them
 */
static esp_err_t init_spi_bus(spi_host_device_t host, int miso, int mosi, int clk)
{
    spi_bus_config_t bus_cfg = bus_configuration(miso, mosi, clk); 
    // Uses the passed 'host' variable instead of hardcoded SPI2/SPI3
    return spi_bus_initialize(host, &bus_cfg, SPI_DMA_CH_AUTO);
}

// --- Public API Implementation ---

/**
 * Performs a single SPI transaction:
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
    gpio_reset_pin(ce_pin);
    gpio_set_direction(ce_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(ce_pin, 0);

    // Configure SPI Device
    spi_device_interface_config_t cfg = device_configuration(csn_pin);
    
    // Add device to bus
    esp_err_t err = spi_bus_add_device(host, &cfg, &out->spi);
    if (err != ESP_OK) return err;

    out->ce_pin = ce_pin;
    out->csn_pin = csn_pin;
    return ESP_OK;
}

/**
 * Sets the power bit to 1 and also the crc and rx bits
 */
void nrf_power_up(nrf24_t *nrf)
{
    write_nrf_register(nrf, NRF_REG_CONFIG, 0X0B);
    esp_rom_delay_us(2000);
}

/**
 * disables enhanced shockburst and sets data rate to 1Mbps
 */
void nrf_init(nrf24_t *nrf)
{
    uint8_t address = 0x01; 
    uint8_t value = 0x00; 
    write_nrf_register(nrf, address, value);

    value = 0x0E; 
    address = 0x06; // RF_SETUP register
    write_nrf_register(nrf, address, value);

    nrf_power_up(nrf);

    // start listening
    gpio_set_level(nrf->ce_pin, 1);
}

/**
 * one spectrum sweep that adds hits when noise is detected
 */
void nrf_scan_band(nrf24_t *nrf, uint16_t *hit_counter, size_t len)
{
    for (uint8_t channel = 0; channel < len && channel < 126; channel++) {
        // switch channel
        write_nrf_register(nrf, NRF_REG_RF_CH, channel);
        
        esp_rom_delay_us(150); 
        
        // read RPD (bit 0)
        uint8_t rpd = read_nrf_register(nrf, NRF_REG_RPD);
        if (rpd & 0x01) {
            hit_counter[channel]++;
        }
    }
}

esp_err_t setup_nrf_interface(nrf24_t *radio, nrf_config_t config) 
{
    // initialize the SPI Bus
    esp_err_t err = init_spi_bus(config.host, config.miso, config.mosi, config.sclk);
    if (err != ESP_OK) return err;

    // attach the NRF device to that bus
    err = nrf_attach(config.host, config.csn, config.ce, radio);
    if (err != ESP_OK) return err;

    // set the internal pins
    radio->ce_pin = config.ce;
    radio->csn_pin = config.csn;
    radio->host = config.host; 

    // power up and init the radio hardware
    nrf_init(radio);
    
    return ESP_OK;
}