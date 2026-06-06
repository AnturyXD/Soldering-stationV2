#ifndef LOCAL_ARDUBOY2_H
#define LOCAL_ARDUBOY2_H

#include <Arduino.h>
#include <Print.h>
#include <Wire.h>

#ifndef WHITE
#define WHITE 1
#endif
#ifndef BLACK
#define BLACK 0
#endif

class Arduboy2 : public Print {
public:
  static const uint8_t WIDTH = 128;
  static const uint8_t HEIGHT = 64;

  Arduboy2() : cursorX(0), cursorY(0), textSize(1), textColor(WHITE),
               textBackground(BLACK), frameRate(30), lastFrame(0), eachFrameMillis(33) {}

  void begin() {
    Wire.begin();
    delay(40);
    command(0xAE);
    command(0xD5); command(0x80);
    command(0xA8); command(0x3F);
    command(0xD3); command(0x00);
    command(0x40);
    command(0x8D); command(0x14);
    command(0x20); command(0x00);
    command(0xA1);
    command(0xC8);
    command(0xDA); command(0x12);
    command(0x81); command(0xCF);
    command(0xD9); command(0xF1);
    command(0xDB); command(0x40);
    command(0xA4);
    command(0xA6);
    clear();
    display();
    command(0xAF);
  }

  void setFrameRate(uint8_t rate) {
    frameRate = rate ? rate : 30;
    eachFrameMillis = 1000UL / frameRate;
  }

  bool nextFrame() {
    unsigned long now = millis();
    if (now - lastFrame < eachFrameMillis) return false;
    lastFrame = now;
    return true;
  }

  void flipVertical(bool enabled) {
    command(enabled ? 0xC0 : 0xC8);
  }

  void flipHorizontal(bool enabled) {
    command(enabled ? 0xA0 : 0xA1);
  }

  void clear() {
    memset(buffer, 0, sizeof(buffer));
  }

  void display() {
    for (uint8_t page = 0; page < 8; page++) {
      command(0xB0 | page);
      command(0x00);
      command(0x10);
      for (uint8_t x = 0; x < WIDTH; x += 16) {
        Wire.beginTransmission(0x3C);
        Wire.write(0x40);
        for (uint8_t i = 0; i < 16; i++) {
          Wire.write(buffer[page * WIDTH + x + i]);
        }
        Wire.endTransmission();
      }
    }
  }

  void drawPixel(int16_t x, int16_t y, uint8_t color = WHITE) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
    uint16_t index = x + (y / 8) * WIDTH;
    uint8_t mask = _BV(y & 7);
    if (color) buffer[index] |= mask;
    else buffer[index] &= ~mask;
  }

  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint8_t color = WHITE) {
    for (int16_t i = 0; i < w; i++) drawPixel(x + i, y, color);
  }

  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint8_t color = WHITE) {
    for (int16_t i = 0; i < h; i++) drawPixel(x, y + i, color);
  }

  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color = WHITE) {
    int16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int16_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy;
    while (true) {
      drawPixel(x0, y0, color);
      if (x0 == x1 && y0 == y1) break;
      int16_t e2 = err * 2;
      if (e2 >= dy) { err += dy; x0 += sx; }
      if (e2 <= dx) { err += dx; y0 += sy; }
    }
  }

  void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color = WHITE) {
    drawFastHLine(x, y, w, color);
    drawFastHLine(x, y + h - 1, w, color);
    drawFastVLine(x, y, h, color);
    drawFastVLine(x + w - 1, y, h, color);
  }

  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color = WHITE) {
    for (int16_t yy = y; yy < y + h; yy++) {
      drawFastHLine(x, yy, w, color);
    }
  }

  void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t w, uint8_t h, uint8_t color = WHITE) {
    drawSlowXYBitmap(x, y, bitmap, w, h, color);
  }

  void drawSlowXYBitmap(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t w, uint8_t h, uint8_t color = WHITE) {
    uint16_t byteWidth = (w + 7) / 8;
    for (uint8_t yy = 0; yy < h; yy++) {
      for (uint8_t xx = 0; xx < w; xx++) {
        if (pgm_read_byte(bitmap + yy * byteWidth + xx / 8) & (0x80 >> (xx & 7))) {
          drawPixel(x + xx, y + yy, color);
        }
      }
    }
  }

  void setCursor(int16_t x, int16_t y) {
    cursorX = x;
    cursorY = y;
  }

  void setTextSize(uint8_t size) {
    textSize = size ? size : 1;
  }

  void setTextColor(uint8_t color) {
    textColor = color;
  }

  void setTextBackground(uint8_t color) {
    textBackground = color;
  }

  size_t write(uint8_t c) override {
    if (c == '\n') {
      cursorX = 0;
      cursorY += textSize * 8;
      return 1;
    }
    if (c == '\r') return 1;
    drawChar(cursorX, cursorY, c, textColor, textBackground, textSize);
    cursorX += textSize * 6;
    return 1;
  }

private:
  uint8_t buffer[WIDTH * HEIGHT / 8];
  int16_t cursorX;
  int16_t cursorY;
  uint8_t textSize;
  uint8_t textColor;
  uint8_t textBackground;
  uint8_t frameRate;
  unsigned long lastFrame;
  unsigned long eachFrameMillis;

  void command(uint8_t value) {
    Wire.beginTransmission(0x3C);
    Wire.write(0x00);
    Wire.write(value);
    Wire.endTransmission();
  }

  void drawChar(int16_t x, int16_t y, uint8_t c, uint8_t color, uint8_t bg, uint8_t size) {
    if (bg != color) fillRect(x, y, 6 * size, 8 * size, bg);
    if (c >= '0' && c <= '9') {
      drawDigit(x, y, c - '0', color, size);
      return;
    }
    if (c == '.') { fillRect(x + 2 * size, y + 6 * size, size, size, color); return; }
    if (c == '%') { drawLine(x, y + 7 * size, x + 5 * size, y, color); drawPixel(x, y, color); drawPixel(x + 5 * size, y + 7 * size, color); return; }
    if (c == '-' || c == '_') { fillRect(x + size, y + 4 * size, 4 * size, size, color); return; }
    if (c == ' ') return;
    drawAlphaBox(x, y, c, color, size);
  }

  void segment(int16_t x, int16_t y, uint8_t seg, uint8_t color, uint8_t size) {
    switch (seg) {
      case 0: fillRect(x + size, y, 3 * size, size, color); break;
      case 1: fillRect(x + 4 * size, y + size, size, 2 * size, color); break;
      case 2: fillRect(x + 4 * size, y + 4 * size, size, 2 * size, color); break;
      case 3: fillRect(x + size, y + 6 * size, 3 * size, size, color); break;
      case 4: fillRect(x, y + 4 * size, size, 2 * size, color); break;
      case 5: fillRect(x, y + size, size, 2 * size, color); break;
      case 6: fillRect(x + size, y + 3 * size, 3 * size, size, color); break;
    }
  }

  void drawDigit(int16_t x, int16_t y, uint8_t digit, uint8_t color, uint8_t size) {
    static const uint8_t masks[10] PROGMEM = {
      0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110,
      0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01101111
    };
    uint8_t mask = pgm_read_byte(&masks[digit]);
    for (uint8_t i = 0; i < 7; i++) {
      if (mask & _BV(i)) segment(x, y, i, color, size);
    }
  }

  void drawAlphaBox(int16_t x, int16_t y, uint8_t c, uint8_t color, uint8_t size) {
    drawRect(x, y, 5 * size, 7 * size, color);
    if (c & 1) drawFastHLine(x + size, y + 3 * size, 3 * size, color);
    if (c & 2) drawFastVLine(x + 2 * size, y + size, 5 * size, color);
  }
};

#endif
