/*
* The MIT License (MIT)
*
* Copyright (c) David Richmond
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
********************************************************************************/

#include <stdio.h>
#include <string.h>
#include "max7219.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "sdkconfig.h"
#include "esp_log.h"

#include <functional>

static const char TAG[] = "MAX7219";

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

Max7219::Max7219(int spi_device, gpio_num_t sck, gpio_num_t mosi, gpio_num_t cs)
    : spi_dev(spi_device), sck(sck), mosi(mosi), cs(cs) {
    this->begin();
}

void Max7219::cs_high(){
    //ESP_EARLY_LOGV(TAG, "cs high %d", this->cs);
    gpio_set_level(this->cs, 1);
}
void Max7219::cs_low(){
    //ESP_EARLY_LOGV(TAG, "cs low %d", this->cs);
    gpio_set_level(this->cs, 0);
}

void Max7219::shutdownStop(void){
    write(REG_SHUTDOWN, 1);
}

// borrowed from https://stackoverflow.com/questions/28746744/passing-capturing-lambda-as-function-pointer
template<typename Function>
struct function_traits;

template <typename Ret, typename... Args>
struct function_traits<Ret(Args...)> {
    typedef Ret(*ptr)(Args...);
};

template <typename Ret, typename... Args>
struct function_traits<Ret(*const)(Args...)> : function_traits<Ret(Args...)> {};

template <typename Cls, typename Ret, typename... Args>
struct function_traits<Ret(Cls::*)(Args...) const> : function_traits<Ret(Args...)> {};

using voidfun = void(*)();

template <typename F>
voidfun lambda_to_void_function(F lambda) {
    static auto lambda_copy = lambda;

    return []() {
        lambda_copy();
    };
}
template <typename F>
auto lambda_to_pointer(F lambda) -> typename function_traits<decltype(&F::operator())>::ptr {
    static auto lambda_copy = lambda;
    
    return []<typename... Args>(Args... args) {
        return lambda_copy(args...);
    };
}
// end borrow 

esp_err_t Max7219::begin(void){
    esp_err_t err;

    // I know this all isn't pretty, but it's just bringing up the spi bus
    // so it's expected to be just a long bunch of init struct and call
    // init function
    ESP_LOGI(TAG, "Initialising SPI bus...");
    spi_bus_config_t bus_config = {};
    bus_config.mosi_io_num = this->mosi;
    bus_config.miso_io_num = -1;
    bus_config.sclk_io_num = this->sck;
    bus_config.max_transfer_sz = 32;
    err = spi_bus_initialize(get_spi(this->spi_dev), &bus_config, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t dev_config = {};
    dev_config.mode = 0;
    dev_config.clock_speed_hz = 10'000'000;
    dev_config.spics_io_num = -1;
    dev_config.flags = 0;
    dev_config.queue_size = 1;
    //std::function<void(spi_transaction_t *)> pre_cb = [&](spi_transaction_t *t) -> void { cs_low(); };
    //std::function<void(spi_transaction_t *)> post_cb = [&](spi_transaction_t *t) -> void { cs_high(); };
    
    dev_config.pre_cb = lambda_to_pointer([&](spi_transaction_t *t){ this->cs_low(); });
    dev_config.post_cb = lambda_to_pointer([&](spi_transaction_t *t){ this->cs_high(); });

    err = spi_bus_add_device(get_spi(this->spi_dev), &dev_config, &this->spi_handle);

    if(err != ESP_OK){
        if(this->spi_handle){
            spi_bus_remove_device(this->spi_handle);
        }   
        return err;
    }

    ESP_LOGI(TAG, "Initialising SPI CS GPIO %d...", this->cs);
    gpio_set_level(this->cs, 1);
    gpio_config_t cs_config = { };
    cs_config.pin_bit_mask = BIT64(this->cs);
    cs_config.mode = GPIO_MODE_OUTPUT;
    gpio_config(&cs_config);

    // now the bus is up, configure the registers
    write(REG_SCAN_LIMIT, 7);
    write(REG_DECODE, 0);
    shutdownStop();
    clear();
    write(REG_BRIGHTNESS, 0x07); // FIXME: make brightness configurable

    return ESP_OK;
}

void Max7219::write(uint8_t opcode, uint8_t data){
    // thankfully we can just set up a transaction to write two bytes onto the spi bus
    // and there's no way to know if it worked other than if the bus itself completely breaks
    uint8_t buffer[2] = { opcode, data };

    ESP_LOGD(TAG, "SPI Trasaction (%x:%x)", opcode, data);
    spi_transaction_t transaction = {};
    transaction.length = 16;
    transaction.tx_buffer = &buffer;
    esp_err_t err = spi_device_polling_transmit(this->spi_handle, &transaction);

    if(err != ESP_OK){
        ESP_LOGE(TAG, "SPI Transaction failed!");
    }
}

void Max7219::clear(void){
    // easy, just zero out all the segment ram (ram starts at address 0x01)
    for(int i = 0; i < 8; i++){
        write(i+1, 0);
    }
}

uint8_t Max7219::lookup_code(char c, bool dp){
    uint8_t d = 0;

    // transform lower casae to upper case, let the user know by setting the dot point
    if(c >= 'a' && c <= 'z'){
        c -= 32;
        dp = 1;
    }

    ESP_LOGD(TAG, "Lookup char 0x%x", c);
    for(int i = 0; MAX7219_Font[i].ascii; i++){
        if(c == MAX7219_Font[i].ascii){
            ESP_LOGD(TAG, "Found char %c at position %d -> %x", c, i, MAX7219_Font[i].segs);
            if(dp){
                d = 1 << 7; // dot point is the 7th segment
            }
            return MAX7219_Font[i].segs;
        }
    }
    
    // well there's no *known* character, just make it empty
    return 0;
}

void Max7219::displayChar(int pos, char c, bool dp){
    ESP_LOGI(TAG, "Display char \"%c\" (%x) at %d", c, c, pos);
    if(pos >= 8) return;
    
    pos = 7-pos; // display is mapped right to left
    write(pos+1, lookup_code(c, dp)); // remember, ram starts at address 0x01
}


void Max7219::displayText(const char *s, Max7219::justification align){
    bool decimals[16] = {0};
    char trimText[16] = "";

    int l = strlen(s);
    if(l > 16) l = 16; // clamp string length

    // go through string looking for dot points and trimming the text where appropriate
    int y = 0;
    for(int i = 0; i < l; i++){
        if(s[i] == '.'){
            decimals[i] = 1;
        } else {
            trimText[y++] = s[i];
        }
    }

    // determine how much we need to offset the text to the right, depending on justification
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

    // now (sorta) blit it all out
    for(int i = 0; i < y && (i + offset) < 8; i++){
        displayChar(i+offset, trimText[i], decimals[i]);
    }
}

// more for testing, just display one decimal number's character at a position on the display
void Max7219::displayDec(int pos, int num){
    if(num > 9) num = 0;
    displayChar((uint8_t)pos, ((uint8_t)(num+'0')), false);
}