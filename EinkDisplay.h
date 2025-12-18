#ifndef _EinkDisplay_H_
#define _EinkDisplay_H_

#include <cstdint>
#include <string>
#include "GFX.h"

#define WHITE                   0
#define BLACK                   1
#define RED                     2

class EinkDisplay : public GFX {
  public:
    // Modified constructor to take device paths/numbers instead of pin numbers
    EinkDisplay(int einkheight, int einkwidth, const std::string& spi_device, int dc_gpio, int rst_gpio, int cs_gpio, int busy_gpio);
    ~EinkDisplay();

    bool         begin();
    void         display(void);
    void         displayNormal(void); // Vendor "Slow" mode
    void         displayFast(void);   // Vendor "Fast" mode
    void         prepare();
    void         clearDisplay(void);
    void         fillBlack(void);
    void         displayImage(const uint8_t* image_bw, const uint8_t* image_red); // New method for full screen image
    void         setWhiteBorder(void);
    void         setBlackBorder(void);
    void         setRedBorder(void);
    void         drawPixel(int16_t x, int16_t y, uint16_t color) override;
    void         drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) override;
    void         drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) override;
    
    int eink_height, eink_width;

  private:
    void _writeCommand(uint8_t command);
    void _writeData(uint8_t data);
    void _sendData(const uint8_t* data, size_t len); // New helper for bulk transfer
    uint8_t _readPixel(int16_t x, int16_t y, uint16_t color);
    void _waitWhileBusy();
    void _beginSPI(void);
    void _endSPI(void);
    
    // Linux specific members
    int spi_fd;
    std::string spi_dev_path;
    int dc_gpio, cs_gpio, busy_gpio, rst_gpio;
    
    // File descriptors for GPIO values to improve performance
    int dc_fd, cs_fd, busy_fd, rst_fd;
    
    uint8_t border = 1;

    // GPIO helpers
    void gpio_export(int gpio);
    void gpio_direction_output(int gpio, int value);
    void gpio_direction_input(int gpio);
    void gpio_set_value(int fd, int value); // Changed to take fd
    int  gpio_get_value(int fd); // Changed to take fd
    int  gpio_open_value(int gpio, int flags); // New helper
    
    // SPI helpers
    void spi_transfer(uint8_t* data, int len);
    uint8_t spi_transfer_byte(uint8_t data);
};

#endif // _EinkDisplay_
