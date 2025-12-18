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
