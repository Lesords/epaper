// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2025 Lese
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>
#include <sys/time.h>
#include "EinkDisplay.h"
#include "image_data.h"

// Default GPIOs (Change these or pass as arguments)
#define DEFAULT_SPI_DEV "/dev/spidev1.0"
#define DEFAULT_BASE_PIN 519
#define DEFAULT_DC_PIN   (DEFAULT_BASE_PIN + 90)
#define DEFAULT_RST_PIN  (DEFAULT_BASE_PIN + 24)
#define DEFAULT_CS_PIN   -1  // -1 means let spidev handle CS
#define DEFAULT_BUSY_PIN (DEFAULT_BASE_PIN + 39)

static long long getTimestampUs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * (long long)(1e+6) + tv.tv_usec);
}

int main(int argc, char* argv[]) {
    std::string spi_dev = DEFAULT_SPI_DEV;
    std::string arg_image;
    int dc = DEFAULT_DC_PIN;
    int rst = DEFAULT_RST_PIN;
    int cs = DEFAULT_CS_PIN;
    int busy = DEFAULT_BUSY_PIN;
    int image_id = 0;
    bool fast_mode = false;
    bool clear_before_fast_mode = false;
    bool full_mode = false;

    // Parse args: ./epaper_test [arg_image] [--fast | --clear] [dc] [rst] [cs] [busy] [test (--full)]
    
    int arg_idx = 1;
    if (argc > arg_idx) arg_image = argv[arg_idx++];
    if (argc > arg_idx) {
        while (arg_idx < argc) {
            if (argv[arg_idx][0] != '-') {
                break;
            }
            if (strcmp(argv[arg_idx], "--fast") == 0) {
                fast_mode = true;
            } else if (strcmp(argv[arg_idx], "--clear") == 0) {
                clear_before_fast_mode = true;
            } else if (strcmp(argv[arg_idx], "--full") == 0) {
                full_mode = true;
            }
            arg_idx++;
        }

        if (argc > arg_idx) dc = std::stoi(argv[arg_idx++]);
        if (argc > arg_idx) rst = std::stoi(argv[arg_idx++]);
        if (argc > arg_idx) cs = std::stoi(argv[arg_idx++]);
        if (argc > arg_idx) busy = std::stoi(argv[arg_idx++]);
    }

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
    } else if (arg_image == "test") {
        image_id = 7;
    } else if (arg_image == "white") {
        image_id = 8;
    } else if (arg_image == "black") {
        image_id = 9;
    } else {
        printf("Usage: %s [boy girl beaglebone tower eagle_binary eagle_bayer eagle_atkinson white black [test (--full)] --fast --clear]\n", argv[0]);
        return 0;
    }

    std::cout << "Initializing E-Ink Display..." << std::endl;
    std::cout << "SPI: " << spi_dev << std::endl;
    
    long long start_time = getTimestampUs();

    // Initialize display for HINK-E042A162 (4.2 inch, 400x300)
    EinkDisplay display(300, 400, spi_dev, dc, rst, cs, busy);

    if (!display.begin()) {
        std::cerr << "Failed to initialize display!" << std::endl;
        return 1;
    }

    if (clear_before_fast_mode) {
        std::cout << "Preparing display (Init & Power On)..." << std::endl;
        display.prepare();
        display.clearDisplay();
        display.displayNormal(); // Use Normal/Slow for clear
        sleep(2);
    }

    std::cout << "Initializing for Normal Mode Image..." << std::endl;
    display.prepare();

    size_t total_bytes = 300 * 400 / 8;
    uint8_t* buffer = (uint8_t*)malloc(total_bytes);
    switch (image_id) {
        case 0:
            display.displayImage(gImage_bw_boy, (fast_mode) ? NULL : gImage_bw_boy);
            break;
        case 1:
            display.displayImage(gImage_bw_girl, (fast_mode) ? NULL : gImage_bw_girl);
            break;
        case 2:
            display.displayImage(gImage_bw_beaglebone, (fast_mode) ? NULL : gImage_bw_beaglebone);
            break;
        case 3:
            display.displayImage(gImage_bw_tower, (fast_mode) ? NULL : gImage_bw_tower);
            break;
        case 4:
            display.displayImage(gImage_bw_eagle_binary, (fast_mode) ? NULL : gImage_bw_eagle_binary);
            break;
        case 5:
            display.displayImage(gImage_bw_eagle_bayer, (fast_mode) ? NULL : gImage_bw_eagle_bayer);
            break;
        case 6:
            display.displayImage(gImage_bw_eagle_atkinson, (fast_mode) ? NULL : gImage_bw_eagle_atkinson);
            break;
        case 7:
            display.displayTest(full_mode);
            break;
        case 8:
            memset(buffer, 0xFF, total_bytes);
            display.displayImage(buffer, buffer);
            break;
        case 9:
            memset(buffer, 0x00, total_bytes);
            display.displayImage(buffer, buffer);
            break;
        default:
            std::cerr << "Invalid image ID!" << std::endl;
            return 1;
    }

    if (image_id == 7) {
        printf("Test mode complete.\n");
        printf("Total time: %.3f seconds\n", float(getTimestampUs() - start_time) / 1e+6);
        return 0;
    }

    if (fast_mode) {
        std::cout << "Updating display (Fast)..." << std::endl;
        display.displayFast();
    } else {
        std::cout << "Updating display (Normal)..." << std::endl;
        display.displayNormal();
    }

    printf("Total time: %.3f seconds\n", float(getTimestampUs() - start_time) / 1e+6);
    std::cout << "Done!" << std::endl;

    return 0;
}
