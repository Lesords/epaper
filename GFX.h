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

#ifndef _GFX_H
#define _GFX_H

#include <cstdint>
#include <cstdlib>
#include <cstring>

class GFX {
public:
    GFX(int16_t w, int16_t h) : _width(w), _height(h), rotation(0), WIDTH(w), HEIGHT(h) {}
    virtual ~GFX() {}

    virtual void drawPixel(int16_t x, int16_t y, uint16_t color) = 0;
    
    // Basic drawing primitives (optional, can be expanded)
    virtual void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
        for(int16_t i=0; i<w; i++) drawPixel(x+i, y, color);
    }
    virtual void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
        for(int16_t i=0; i<h; i++) drawPixel(x, y+i, color);
    }
    
    int16_t width() const { return _width; }
    int16_t height() const { return _height; }
    uint8_t getRotation() const { return rotation; }
    void setRotation(uint8_t r) { rotation = r; }

protected:
    int16_t _width, _height;
    uint8_t rotation;
    const int16_t WIDTH, HEIGHT;
};

#endif
