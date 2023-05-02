#include "max7219.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

inline void sleep(int milliseconds){
    vTaskDelay(milliseconds / portTICK_PERIOD_MS);
}

extern "C" {
    void app_main(void){
        Max7219 max;
        
        max.begin();
        for(int i = 0; i < 8; i++){
            //max.clear();
            max.displayDec(i, i);
            sleep(1000);
        }
        sleep(1000);
        while(1){
            max.clear();
            max.displayText("HELLO", Max7219::RIGHT);
            sleep(1000);

            max.clear();
            max.displayText("HELLO", Max7219::LEFT);
            sleep(1000);
        }
    }
}