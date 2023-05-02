#include <stdio.h>
#include <string.h>
#include "max7219.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "sdkconfig.h"
#include "esp_log.h"

#ifdef CONFIG_IDF_TARGET_ESP32
#   define ESP_SPI_HOST SPI2_HOST
#elif CONFIG_IDF_TARGET_ESP32S2
#   define ESP_SPI_HOST SPI2_HOST
#elif CONFIG_IDF_TARGET_ESP32S3
#   define ESP_SPI_HOST SPI2_HOST
#elif CONFIG_IDF_TARGET_ESP32C3
#   define ESP_SPI_HOST SPI2_HOST
#else
#   error Unknown esp32 chip type!
#endif

static const char TAG[] = "MAX7219";
static const gpio_num_t cs_gpio = (gpio_num_t)CONFIG_EXAMPLE_SPI_CS;

static const uint8_t REG_DECODE = 0x09;
static const uint8_t REG_BRIGHTNESS = 0x0a;
static const uint8_t REG_SCAN_LIMIT = 0x0b;
static const uint8_t REG_SHUTDOWN = 0x0c;
static const uint8_t REG_TEST = 0x0f;

static const struct {
	char   ascii;
	char   segs;
} MAX7219_Font[] = {
  {'A',0b1110111},{'B',0b1111111},{'C',0b1001110},{'D',0b1111110},{'E',0b1001111},{'F',0b1000111},       
  {'G',0b1011110},{'H',0b0110111},{'I',0b0110000},{'J',0b0111100},{'L',0b0001110},{'N',0b1110110},       
  {'O',0b1111110},{'P',0b1100111},{'R',0b0000101},{'S',0b1011011},{'T',0b0001111},{'U',0b0111110},       
  {'Y',0b0100111},{'[',0b1001110},{']',0b1111000},{'_',0b0001000},{'a',0b1110111},{'b',0b0011111},       
  {'c',0b0001101},{'d',0b0111101},{'e',0b1001111},{'f',0b1000111},{'g',0b1011110},{'h',0b0010111},       
  {'i',0b0010000},{'j',0b0111100},{'l',0b0001110},{'n',0b0010101},{'o',0b1111110},{'p',0b1100111},       
  {'r',0b0000101},{'s',0b1011011},{'t',0b0001111},{'u',0b0011100},{'y',0b0100111},{'-',0b0000001},
  {' ',0b0000000},{'0',0b1111110},{'1',0b0110000},{'2',0b1101101},{'3',0b1111001},{'4',0b0110011},
  {'5',0b1011011},{'6',0b1011111},{'7',0b1110000},{'8',0b1111111},{'9',0b1111011},{'\0',0b0000000},
  };

Max7219::Max7219(void){

}

static void cs_high(spi_transaction_t *t){
    ESP_EARLY_LOGV(TAG, "cs high %d", CONFIG_EXAMPLE_SPI_CS);
    gpio_set_level(cs_gpio, 1);
}
static void cs_low(spi_transaction_t *t){
    ESP_EARLY_LOGV(TAG, "cs low %d", CONFIG_EXAMPLE_SPI_CS);
    gpio_set_level(cs_gpio, 0);
}

void Max7219::shutdownStop(void){
    write(REG_SHUTDOWN, 1);
}
esp_err_t Max7219::begin(void){
    esp_err_t err;


    ESP_LOGI(TAG, "Initialising SPI bus...");
    spi_bus_config_t bus_config = {
        .mosi_io_num = CONFIG_EXAMPLE_SPI_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = CONFIG_EXAMPLE_SPI_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };
    err = spi_bus_initialize(ESP_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t dev_config = {
        .mode = 0,
        .clock_speed_hz = 10'000'000,
        .spics_io_num = -1,
        .flags = 0,
        .queue_size = 1,
        .pre_cb = cs_low,
        .post_cb = cs_high,
    };
    err = spi_bus_add_device(ESP_SPI_HOST, &dev_config, &this->spi_handle);

    if(err != ESP_OK){
        if(this->spi_handle){
            spi_bus_remove_device(this->spi_handle);
        }   
        return err;
    }

    ESP_LOGI(TAG, "Initialising SPI CS GPIO %d...", CONFIG_EXAMPLE_SPI_CS);
    gpio_set_level(cs_gpio, 1);
    gpio_config_t cs_config = {
        .pin_bit_mask = BIT64(CONFIG_EXAMPLE_SPI_CS),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&cs_config);

    write(REG_SCAN_LIMIT, 7);
    write(REG_DECODE, 0);
    shutdownStop();
    clear();
    write(REG_BRIGHTNESS, 0x07);

    return ESP_OK;
}

void Max7219::write(uint8_t opcode, uint8_t data){
    uint8_t buffer[2] = { opcode, data };

    ESP_LOGI(TAG, "SPI Trasaction (%x:%x)", opcode, data);
    spi_transaction_t transaction = {
        .length = 16,
        .tx_buffer = &buffer,
    };
    esp_err_t err = spi_device_polling_transmit(this->spi_handle, &transaction);

    if(err != ESP_OK){
        ESP_LOGE(TAG, "SPI Transaction failed!");
    }
}

void Max7219::clear(void){
    for(int i = 0; i < 8; i++){
        write(i+1, 0);
    }
}

uint8_t Max7219::lookup_code(char c, bool dp){
    uint8_t d = 0;

    if(dp) d = 1;
    if(c >= 'a' && c <= 'z'){
        c -= 32;
        d = 1;
    }
    ESP_LOGI(TAG, "Lookup char 0x%x", c);
    for(int i = 0; MAX7219_Font[i].ascii; i++){
        if(c == MAX7219_Font[i].ascii){
            ESP_LOGI(TAG, "Found char %c at position %d -> %x", c, i, MAX7219_Font[i].segs);
            if(d){
                d = 1 << 7; // dot point is the 7th segment
            }
            return MAX7219_Font[i].segs;
        }
    }
    return 0;
}

void Max7219::displayChar(int pos, char c, bool dp){
    ESP_LOGI(TAG, "Display char \"%c\" (%x) at %d", c, c, pos);
    if(pos >= 8) return;
    
    pos = 7-pos; // display is mapped right to left
    write(pos+1, lookup_code(c, dp));
}


void Max7219::displayText(const char *s, Max7219::justification align){
    bool decimals[16] = {0};
    char trimText[16] = "";

    int l = strlen(s);
    if(l > 16) l = 16; // clamp string length

    int y = 0;
    for(int i = 0; i < l; i++){
        if(s[i] == '.'){
            decimals[i] = 1;
        } else {
            trimText[y++] = s[i];
        }
    }

    int offset = 0;
    if(align == RIGHT || align == CENTRE){
        if(y < 8){
            offset = 8-y;
            if(align == CENTRE){
                offset = offset / 2;
            }
        } else {
            offset = 0;
        }
    }

    for(int i = 0; i < y && (i + offset) < 8; i++){
        displayChar(i+offset, trimText[i], decimals[i]);
    }
}

// more for testing
void Max7219::displayDec(int pos, int num){
    if(num > 9) num = 0;
    displayChar((uint8_t)pos, ((uint8_t)(num+'0')), false);
}