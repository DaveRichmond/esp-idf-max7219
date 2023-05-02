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