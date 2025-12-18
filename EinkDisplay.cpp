#include "EinkDisplay.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <thread>
#include <chrono>

#define xy_swap(a, b) \
  (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b)))

// Helper for delays
static void delay(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

EinkDisplay::EinkDisplay(int einkheight, int einkwidth, const std::string& spi_device, int dc_gpio, int rst_gpio, int cs_gpio, int busy_gpio) :
  GFX(einkwidth, einkheight),
  eink_height(einkheight), eink_width(einkwidth),
  spi_fd(-1),
  spi_dev_path(spi_device),
  dc_gpio(dc_gpio), cs_gpio(cs_gpio), busy_gpio(busy_gpio), rst_gpio(rst_gpio),
  dc_fd(-1), cs_fd(-1), busy_fd(-1), rst_fd(-1)
{
}

EinkDisplay::~EinkDisplay() {
    if (spi_fd >= 0) close(spi_fd);
    if (dc_fd >= 0) close(dc_fd);
    if (cs_fd >= 0) close(cs_fd);
    if (busy_fd >= 0) close(busy_fd);
    if (rst_fd >= 0) close(rst_fd);
}

bool EinkDisplay::begin() {
    // Setup GPIOs
    if (dc_gpio >= 0) {
        gpio_export(dc_gpio);
        gpio_direction_output(dc_gpio, 1);
        dc_fd = gpio_open_value(dc_gpio, O_WRONLY);
    }
    
    if (rst_gpio >= 0) {
        gpio_export(rst_gpio);
        gpio_direction_output(rst_gpio, 1);
        rst_fd = gpio_open_value(rst_gpio, O_WRONLY);
    }

    if (cs_gpio >= 0) {
        gpio_export(cs_gpio);
        gpio_direction_output(cs_gpio, 1);
        cs_fd = gpio_open_value(cs_gpio, O_WRONLY);
    }

    if (busy_gpio >= 0) {
        gpio_export(busy_gpio);
        gpio_direction_input(busy_gpio);
        busy_fd = gpio_open_value(busy_gpio, O_RDONLY);
    }

    if (dc_fd < 0 || rst_fd < 0 || busy_fd < 0) {
        fprintf(stderr, "Failed to open required GPIO value files (DC, RST, BUSY)\n");
        return false;
    }
    // CS is optional if managed by spidev
    if (cs_gpio >= 0 && cs_fd < 0) {
         fprintf(stderr, "Failed to open CS GPIO value file\n");
         return false;
    }

    // Setup SPI
    spi_fd = open(spi_dev_path.c_str(), O_RDWR);
    if (spi_fd < 0) {
        perror("Failed to open SPI device");
        return false;
    }

    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t speed = 1000000; // Lowered to 1MHz for stability

    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0) {
        perror("SPI mode");
        return false;
    }
    if (ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
        perror("SPI bits");
        return false;
    }
    if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        perror("SPI speed");
        return false;
    }

    return true;
}

void EinkDisplay::prepare(void) {
  uint16_t eink_x = ((eink_width - 1) / 8);
  uint16_t eink_y = (eink_height - 1);

  // Hardware Reset
  delay(20);
  gpio_set_value(rst_fd, 0);
  delay(20);
  gpio_set_value(rst_fd, 1);
  delay(30);

  _beginSPI();

  // Software Reset (from main.c in vendor code)
  _writeCommand(0x12);
  _waitWhileBusy();
  delay(5);

  // --- INIT_SSD1683 Sequence Start ---
  
  // Driver Output Control
  _writeCommand(0x01);
  _writeData(eink_y);         // 0x2B (for 300)
  _writeData((eink_y >> 8));  // 0x01
  _writeData(0x00);

  // Data Entry Mode
  _writeCommand(0x11);
  _writeData(0x01); // Y decrement, X increment

  // Set RAM X address Start/End
  _writeCommand(0x44);
  _writeData(0x00);           // Start
  _writeData(eink_x);         // End (0x31 for 400 width / 8 - 1)

  // Set RAM Y address Start/End
  _writeCommand(0x45);
  _writeData(eink_y);         // Start (0x12B)
  _writeData((eink_y >> 8));
  _writeData(0x00);           // End
  _writeData(0x00);

  // Border Waveform Control
  _writeCommand(0x3C);
  _writeData(0x01); // Vendor sets White (0x01)

  // Display Update Control 1
  _writeCommand(0x21);
  _writeData(0x40);

  // --- INIT_SSD1683 Sequence End ---

  // Note: Vendor INIT does NOT include 0x22 or 0x20. 
  // Those are done in the display update functions.
  
  _endSPI();
}

void EinkDisplay::display(void) {
  // Default to Normal/Slow mode if not specified, or keep existing behavior
  // Existing behavior was 0xC7. Vendor Slow is 0xF7.
  // Let's use displayNormal() logic here to be safe, or keep it separate.
  // For now, I'll just call displayNormal() as the default "display" action.
  displayNormal();
}

void EinkDisplay::displayNormal(void) {
  _beginSPI();
  
  _writeCommand(0x18); // Temperature sensor
  _writeData(0x80);    // Internal

  _writeCommand(0x21); // Display Update Control 1
  _writeData(0x40);    // RAM -> Display (Source Output) ? Vendor uses 0x40 here.

  _writeCommand(0x22); // Display Update Control 2
  _writeData(0xF7);    // Vendor "Slow" mode sequence

  _writeCommand(0x20); // Master Activation
  _waitWhileBusy();
  
  // Force a delay to ensure update completes
  delay(3000); // Keep the safety delay

  _writeCommand (0x10); // Deep Sleep mode
  _writeData (0x01);
  _endSPI();
}

void EinkDisplay::displayFast(void) {
  _beginSPI();
  
  _writeCommand(0x18); // Temperature sensor
  _writeData(0x80);    // Internal

  _writeCommand(0x21); // Display Update Control 1
  _writeData(0x00);    // Different from Normal (0x40)

  _writeCommand(0x22); // Display Update Control 2
  _writeData(0xFF);    // Vendor "Fast" mode sequence

  _writeCommand(0x20); // Master Activation
  _waitWhileBusy();
  
  // Force a delay to ensure update completes
  delay(3000); // Keep the safety delay

  _writeCommand (0x10); // Deep Sleep mode
  _writeData (0x01);
  _endSPI();
}

void EinkDisplay::_beginSPI(void)
{
}

void EinkDisplay::_endSPI(void)
{
}

void EinkDisplay::_writeCommand(uint8_t command)
{
  gpio_set_value(dc_fd, 0);
  if (cs_fd >= 0) gpio_set_value(cs_fd, 0);
  spi_transfer_byte(command);
  if (cs_fd >= 0) gpio_set_value(cs_fd, 1);
}

void EinkDisplay::_writeData(uint8_t data)
{
  gpio_set_value(dc_fd, 1);
  if (cs_fd >= 0) gpio_set_value(cs_fd, 0);
  spi_transfer_byte(data);
  if (cs_fd >= 0) gpio_set_value(cs_fd, 1);
}

void EinkDisplay::_sendData(const uint8_t* data, size_t len) {
    if (len == 0) return;
    gpio_set_value(dc_fd, 1);
    if (cs_fd >= 0) gpio_set_value(cs_fd, 0);
    
    // spidev has a limit on buffer size (often 4096 bytes).
    // We should split larger transfers.
    const size_t MAX_CHUNK = 4096;
    size_t offset = 0;
    while (offset < len) {
        size_t chunk = (len - offset > MAX_CHUNK) ? MAX_CHUNK : (len - offset);
        // We need to cast away const because spi_transfer takes non-const (it uses same buf for rx)
        // But here we don't care about RX.
        // Ideally we should copy to a temp buffer if we want to be strictly safe, 
        // but spi_transfer implementation uses it as tx_buf.
        spi_transfer(const_cast<uint8_t*>(data + offset), chunk);
        offset += chunk;
    }

    if (cs_fd >= 0) gpio_set_value(cs_fd, 1);
}

uint8_t EinkDisplay::_readPixel(int16_t x, int16_t y, uint16_t color)
{
  _writeCommand (0x41);//Read RAM option
  _writeData(color);//0 = BW, 1 = RED RAM

  _writeCommand(0x4E);//Set RAM X address counter
  _writeData((x / 8));
  _writeCommand(0x4F);//Set RAM Y address counter
  _writeData(y);
  _writeData((y >> 8));

  if (cs_fd >= 0) gpio_set_value(cs_fd, 0);
  gpio_set_value(dc_fd, 0);
  spi_transfer_byte(0x27);
  gpio_set_value(dc_fd, 1);
  
  uint8_t result = spi_transfer_byte(0x27); // dummy read?
  result = spi_transfer_byte(0x27); // actual data
  
  if (cs_fd >= 0) gpio_set_value(cs_fd, 1);
  return result;
}

void EinkDisplay::_waitWhileBusy()
{
  while (gpio_get_value(busy_fd) == 1) {
      delay(1);
  }
}

void EinkDisplay::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x >= 0) && (x < width()) && (y >= 0) && (y < height())) {
    switch (getRotation()) {
      case 1:
        xy_swap(x, y);
        x = WIDTH - x - 1;
        break;
      case 2:
        x = WIDTH  - x - 1;
        y = HEIGHT - y - 1;
        break;
      case 3:
        xy_swap(x, y);
        y = HEIGHT - y - 1;
        break;
    }

    uint8_t eink_x = (x / 8);
    uint16_t eink_y = y;
    uint8_t curr_black;
    uint8_t curr_red;

    _beginSPI();
    curr_black = _readPixel(x, y, 0); //read black pixel
    curr_red = _readPixel(x, y, 1); //read red pixel

    curr_black = (curr_black & (0xFF ^ (1 << (7 - x % 8))));
    curr_red = (curr_red & (0xFF ^ (1 << (7 - x % 8))));

    if (color == BLACK) curr_black = (curr_black | (1 << (7 - x % 8)));
    if (color == RED) curr_red = (curr_red | (1 << (7 - x % 8)));

    //Writing Black/Wide pixel
    _writeCommand(0x4E);//Set RAM X address counter
    _writeData(eink_x);
    _writeCommand(0x4F);//Set RAM Y address counter
    _writeData(eink_y);
    _writeData((eink_y >> 8));

    _writeCommand(0x24);//WRITE RAM BW
    _writeData(curr_black);

    _writeCommand(0x26);//WRITE RAM RED
    _writeData(curr_red);
    _endSPI();
  }
}

void EinkDisplay::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    for(int16_t i=0; i<w; i++) drawPixel(x+i, y, color);
}

void EinkDisplay::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    for(int16_t i=0; i<h; i++) drawPixel(x, y+i, color);
}

void EinkDisplay::clearDisplay(void) {
    uint16_t w = (eink_width + 7) / 8;
    uint16_t h = eink_height;
    size_t total_bytes = w * h;
    
    // Allocate a buffer for one full frame (or at least a large chunk)
    // For 400x300, w=50, h=300, total=15000 bytes.
    // We can allocate this on heap.
    uint8_t* buffer = (uint8_t*)malloc(total_bytes);
    if (!buffer) return;

    _beginSPI();
    
    // White for BW RAM (0xFF)
    memset(buffer, 0xFF, total_bytes);
    _writeCommand(0x24);
    _sendData(buffer, total_bytes);

    // No Red (0x00)
    memset(buffer, 0x00, total_bytes);
    _writeCommand(0x26);
    _sendData(buffer, total_bytes);
    
    _endSPI();
    free(buffer);
}

void EinkDisplay::fillBlack(void) {
    uint16_t w = (eink_width + 7) / 8;
    uint16_t h = eink_height;
    size_t total_bytes = w * h;
    
    uint8_t* buffer = (uint8_t*)malloc(total_bytes);
    if (!buffer) return;

    _beginSPI();
    
    // Black for BW RAM (0x00)
    memset(buffer, 0x00, total_bytes);
    _writeCommand(0x24);
    _sendData(buffer, total_bytes);

    // No Red (0x00)
    memset(buffer, 0x00, total_bytes);
    _writeCommand(0x26);
    _sendData(buffer, total_bytes);
    
    _endSPI();
    free(buffer);
}

void EinkDisplay::displayImage(const uint8_t* image_bw, const uint8_t* image_red) {
    uint16_t w = (eink_width + 7) / 8;
    uint16_t h = eink_height;
    size_t total_bytes = w * h;

    _beginSPI();

    // Set RAM X address counter to 0
    _writeCommand(0x4E);
    _writeData(0x00);
    
    // Set RAM Y address counter to eink_height - 1
    _writeCommand(0x4F);
    _writeData(eink_height - 1);
    _writeData((eink_height - 1) >> 8);

    // BW RAM
    _writeCommand(0x24);
    if (image_bw) {
        // Vertical Flip (Fix for "Up/Down Left/Right reversed")
        uint8_t* buffer = (uint8_t*)malloc(total_bytes);
        if (buffer) {
            for (int y = 0; y < h; y++) {
                // Copy row 'y' from source to row 'h-1-y' in destination
                memcpy(buffer + (h - 1 - y) * w, image_bw + y * w, w);
            }
            _sendData(buffer, total_bytes);
            free(buffer);
        } else {
            _sendData(image_bw, total_bytes);
        }
    } else {
        // Default to white if null
        uint8_t* buffer = (uint8_t*)malloc(total_bytes);
        if (buffer) {
            memset(buffer, 0xFF, total_bytes);
            _sendData(buffer, total_bytes);
            free(buffer);
        }
    }

    // Reset RAM counters for Red layer
    _writeCommand(0x4E);
    _writeData(0x00);
    _writeCommand(0x4F);
    _writeData(eink_height - 1);
    _writeData((eink_height - 1) >> 8);

    // Red RAM
    _writeCommand(0x26);
    if (image_red) {
        // Vertical Flip for Red channel too
        uint8_t* buffer = (uint8_t*)malloc(total_bytes);
        if (buffer) {
            for (int y = 0; y < h; y++) {
                memcpy(buffer + (h - 1 - y) * w, image_red + y * w, w);
            }
            _sendData(buffer, total_bytes);
            free(buffer);
        } else {
            _sendData(image_red, total_bytes);
        }
    } else {
        // Default to no red if null
        uint8_t* buffer = (uint8_t*)malloc(total_bytes);
        if (buffer) {
            memset(buffer, 0x00, total_bytes);
            _sendData(buffer, total_bytes);
            free(buffer);
        }
    }

    _endSPI();
}

void EinkDisplay::setWhiteBorder(void) { border = 1; }
void EinkDisplay::setBlackBorder(void) { border = 0; }
void EinkDisplay::setRedBorder(void) { border = 2; }

// --- GPIO Helpers (Sysfs) ---

void EinkDisplay::gpio_export(int gpio) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio);
    if (access(path, F_OK) == 0) return; // Already exported

    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0) return;
    char buf[16];
    int len = snprintf(buf, sizeof(buf), "%d", gpio);
    write(fd, buf, len);
    close(fd);
    delay(100); // Wait for udev
}

void EinkDisplay::gpio_direction_output(int gpio, int value) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio);
    int fd = open(path, O_WRONLY);
    if (fd < 0) return;
    write(fd, "out", 3);
    close(fd);
    
    // Set initial value
    int val_fd = gpio_open_value(gpio, O_WRONLY);
    if (val_fd >= 0) {
        gpio_set_value(val_fd, value);
        close(val_fd);
    }
}

void EinkDisplay::gpio_direction_input(int gpio) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio);
    int fd = open(path, O_WRONLY);
    if (fd < 0) return;
    write(fd, "in", 2);
    close(fd);
}

int EinkDisplay::gpio_open_value(int gpio, int flags) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio);
    return open(path, flags);
}

void EinkDisplay::gpio_set_value(int fd, int value) {
    if (fd < 0) return;
    write(fd, value ? "1" : "0", 1);
}

int EinkDisplay::gpio_get_value(int fd) {
    if (fd < 0) return 0;
    char buf[2];
    lseek(fd, 0, SEEK_SET); // Rewind to read again
    read(fd, buf, 1);
    return (buf[0] == '1') ? 1 : 0;
}

// --- SPI Helpers ---

uint8_t EinkDisplay::spi_transfer_byte(uint8_t data) {
    uint8_t rx = 0;
    struct spi_ioc_transfer tr;
    memset(&tr, 0, sizeof(tr));
    tr.tx_buf = (unsigned long)&data;
    tr.rx_buf = (unsigned long)&rx;
    tr.len = 1;
    tr.speed_hz = 4000000;
    tr.bits_per_word = 8;
    
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        // Only print error once to avoid flooding logs
        static bool error_printed = false;
        if (!error_printed) {
            perror("SPI transfer failed");
            fprintf(stderr, "Error code: %d\n", errno);
            error_printed = true;
        }
    }
    return rx;
}

void EinkDisplay::spi_transfer(uint8_t* data, int len) {
    struct spi_ioc_transfer tr;
    memset(&tr, 0, sizeof(tr));
    tr.tx_buf = (unsigned long)data;
    tr.rx_buf = 0;
    tr.len = len;
    tr.speed_hz = 4000000;
    tr.bits_per_word = 8;
    
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
         static bool error_printed = false;
        if (!error_printed) {
            perror("SPI transfer failed");
            fprintf(stderr, "Error code: %d\n", errno);
            error_printed = true;
        }
    }
}
