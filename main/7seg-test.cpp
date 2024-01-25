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
#include "max7219.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string>

inline void sleep(int milliseconds){
    vTaskDelay(milliseconds / portTICK_PERIOD_MS);
}

const auto CLK_PIN = static_cast<gpio_num_t>(4);
const auto CS_PIN = static_cast<gpio_num_t>(5);
const auto MOSI_PIN = static_cast<gpio_num_t>(6);

extern "C" {
    void app_main(void){
        Max7219 max(1, CLK_PIN, MOSI_PIN, CS_PIN);
        
        for(int i = 0; i < 8; i++){
            //max.clear();
            max.displayDec(i, i);
            sleep(250);
        }
        sleep(1000);

        for(int i = 0; i < 5; i++){
            max.clear();
            max.displayText("HELLO", Max7219::RIGHT);
            sleep(500);

            max.clear();
            max.displayText("HELLO", Max7219::LEFT);
            sleep(500);
        }

        max.clear();
        int count = 0;
        while(1){
            std::string s = std::to_string(count++);
            max.displayText(s.c_str(), Max7219::RIGHT);
            sleep(50);
        }
    }
}