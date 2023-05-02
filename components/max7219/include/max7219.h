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

#include "driver/spi_master.h"

class Max7219 {
    private:
        spi_device_handle_t spi_handle;
        void write(uint8_t opcode, uint8_t data);
        uint8_t lookup_code(char c, bool dp);
        void shutdownStop(void);

    public:
        enum justification {
            LEFT,
            RIGHT,
            CENTRE
        } ;
        
        Max7219(void);
        esp_err_t begin(void);
        void clear(void);
        void displayChar(int pos, char c, bool dp);
        void displayText(const char *str, justification align);
        void displayDec(int pos, int num);
};