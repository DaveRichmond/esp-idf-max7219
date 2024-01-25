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

#include <stdint.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"

class Max7219 {
    private:
        constexpr spi_host_device_t get_spi(int n){
           switch(n){
                case 0: return SPI1_HOST;
                case 1: return SPI2_HOST;
                #ifdef SPI3_HOST
                case 2: return SPI3_HOST;
                #endif
                default: return SPI_HOST_MAX;
            }
        }
        spi_device_handle_t spi_handle;
        void write(uint8_t opcode, uint8_t data);
        uint8_t lookup_code(char c, bool dp);
        void shutdownStop(void);

        void cs_high(void);
        void cs_low(void);


        int spi_dev;
        gpio_num_t sck, mosi, cs;

    public:
        enum justification {
            LEFT,
            RIGHT,
            CENTRE
        } ;
        
        Max7219(int spi_device, gpio_num_t sck, gpio_num_t mosi, gpio_num_t cs);
        esp_err_t begin(void);
        void clear(void);
        void displayChar(int pos, char c, bool dp);
        void displayText(const char *str, justification align);
        void displayDec(int pos, int num);
};