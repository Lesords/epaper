// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2025 Lese
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
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
