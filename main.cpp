#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>
#include "EinkDisplay.h"
#include "image_data.h"

// Default GPIOs (Change these or pass as arguments)
// These are just placeholders!
#define DEFAULT_SPI_DEV "/dev/spidev1.0"
#define DEFAULT_BASE_PIN 519
#define DEFAULT_DC_PIN   (DEFAULT_BASE_PIN + 90)
#define DEFAULT_RST_PIN  (DEFAULT_BASE_PIN + 24)
#define DEFAULT_CS_PIN   -1  // -1 means let spidev handle CS
#define DEFAULT_BUSY_PIN (DEFAULT_BASE_PIN + 39)

int main(int argc, char* argv[]) {
    std::string spi_dev = DEFAULT_SPI_DEV;
    std::string arg_image;
    int dc = DEFAULT_DC_PIN;
    int rst = DEFAULT_RST_PIN;
    int cs = DEFAULT_CS_PIN;
    int busy = DEFAULT_BUSY_PIN;
    int image_id = 0;

    // Parse args: ./epaper_test [arg_image] [dc] [rst] [cs] [busy]
    
    int arg_idx = 1;
    if (argc > arg_idx) arg_image = argv[arg_idx++];
    if (argc > arg_idx) dc = std::stoi(argv[arg_idx++]);
    if (argc > arg_idx) rst = std::stoi(argv[arg_idx++]);
    if (argc > arg_idx) cs = std::stoi(argv[arg_idx++]);
    if (argc > arg_idx) busy = std::stoi(argv[arg_idx++]);

    if (arg_image == "boy") {
        image_id = 0;
    } else if (arg_image == "girl") {
        image_id = 1;
    } else if (arg_image == "beaglebone") {
        image_id = 2;
    } else if (arg_image == "tower") {
        image_id = 3;
    } else if (arg_image == "eagle_binary") {
        image_id = 4;
    } else if (arg_image == "eagle_bayer") {
        image_id = 5;
    } else if (arg_image == "eagle_atkinson") {
        image_id = 6;
    } else {
        printf("Usage: %s [boy girl beaglebone tower eagle_binary eagle_bayer eagle_atkinson]\n", argv[0]);
        return 0;
    }

    std::cout << "Initializing E-Ink Display..." << std::endl;
    std::cout << "SPI: " << spi_dev << std::endl;
    
    // Initialize display for HINK-E042A162 (4.2 inch, 400x300)
    EinkDisplay display(300, 400, spi_dev, dc, rst, cs, busy);

    if (!display.begin()) {
        std::cerr << "Failed to initialize display!" << std::endl;
        return 1;
    }

    std::cout << "Preparing display (Init & Power On)..." << std::endl;
    display.prepare();

    // --- Added Clear Cycle to remove ghosting/artifacts ---
    std::cout << "Clearing display (White refresh)..." << std::endl;
    display.clearDisplay();
    display.displayNormal(); // Use Normal/Slow for clear
    
    // Display goes to sleep after display(), so we must wake it up again
    std::cout << "Re-initializing for Normal Mode Image..." << std::endl;
    display.prepare();
    // -----------------------------------------------------

    std::cout << "Displaying image (Normal/Slow Mode)..." << std::endl;

    switch (image_id) {
        case 0:
            display.displayImage(gImage_bw_boy, NULL);
            break;
        case 1:
            display.displayImage(gImage_bw_girl, NULL);
            break;
        case 2:
            display.displayImage(gImage_bw_beaglebone, NULL);
            break;
        case 3:
            display.displayImage(gImage_bw_tower, NULL);
            break;
        case 4:
            display.displayImage(gImage_bw_eagle_binary, NULL);
            break;
        case 5:
            display.displayImage(gImage_bw_eagle_bayer, NULL);
            break;
        case 6:
            display.displayImage(gImage_bw_eagle_atkinson, NULL);
            break;
        default:
            std::cerr << "Invalid image ID!" << std::endl;
            return 1;
    }

    // Pass NULL for red channel to avoid displaying old/garbage data
    // display.displayImage(gImage_bw_beaglebone, NULL);
    
    std::cout << "Updating display (Normal)..." << std::endl;
    display.displayNormal();
    
    std::cout << "Waiting 1 seconds..." << std::endl;
    sleep(1);

    return 0;

    std::cout << "Re-initializing for Fast Mode Image..." << std::endl;
    display.prepare();

    std::cout << "Displaying image (Fast Mode)..." << std::endl;
    // We can just update again with the same image data in RAM, 
    // but displayImage writes to RAM. 
    // Since the display powered down, RAM might be lost? 
    // SSD1683 RAM is usually retained if VDD is kept, but Deep Sleep (0x10, 0x01) usually turns off power.
    // So we should rewrite the image.
    display.displayImage(gImage_bw_beaglebone, NULL);

    std::cout << "Updating display (Fast)..." << std::endl;
    display.displayFast();

    std::cout << "Done!" << std::endl;

    return 0;
}
