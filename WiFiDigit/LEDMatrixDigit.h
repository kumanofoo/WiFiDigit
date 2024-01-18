#ifndef LEDMatrixDigit_h
#define LEDMatrixDigit_h
#include "number_fonts.h"
#define MATRIX_WIDTH 12
#define MATRIX_HEIGHT 8
#define FRAME_WIDTH 32
#define FRAME_HEIGHT 3

class LEDMatrixDigit {
public:
  LEDMatrixDigit();
  void begin(ArduinoLEDMatrix matrix);
  void print(int number);
  void save0();
  void load0();
  void print0();
  void setDot(int x, int y, bool value);
  int blink();
  int blinkInvert();
  void clearMatrix();
  void animatedClear();
  void loadBitmap(uint16_t bitmap[MATRIX_HEIGHT]);
  void testPattern();
  void upsideDown(bool upside_down);
  
private:
  uint16_t _bitmap[MATRIX_HEIGHT];
  uint16_t _bitmap0[MATRIX_HEIGHT];
  uint16_t _bitmap_blank[MATRIX_HEIGHT];
  int _blink_flag = 0;
  bool _upside_down;
  ArduinoLEDMatrix _matrix;
  void _bitmap2frame(uint32_t frame[FRAME_HEIGHT]);
  void _setMatrix(uint16_t bitmap[MATRIX_HEIGHT]);
  void _setMatrixInvert(uint16_t bitmap[MATRIX_HEIGHT]);
  void _reverseBits(uint16_t x[MATRIX_HEIGHT], uint16_t r[MATRIX_HEIGHT]);
  uint16_t _reverseBits(uint16_t x);
};

/**
 * WiFiDigit constructor
 */
LEDMatrixDigit::LEDMatrixDigit() {
  for (int i = 0; i < MATRIX_HEIGHT; i++) {
    _bitmap[i] = 0;
    _bitmap0[i] = 0;
    _bitmap_blank[i] = 0;    
  }
  _upside_down = false;
}

/**
 * Setup LEDMatrixDigit
 *
 * @param signed integer with 2 digit.
 */
void LEDMatrixDigit::begin(ArduinoLEDMatrix matrix) {
  _matrix = matrix;
}

/**
 * Display number with 2 digit on LED matrix (from -99 to 99).
 *
 * @param signed integer with 2 digit.
 */
void LEDMatrixDigit::print(int number) {
  if (number > 99 || number < -99) return;
  int sign = number < 0 ? 1 : 0;
  int n = number < 0 ? number * -1 : number;
  int ten = n / 10;
  int one = n % 10;
  _bitmap[0] = 0;
  _bitmap[1] = 0;
  int row = 1;
  int right_spacing = 0;
  if (sign == 0) {
    right_spacing += 1;
  }
  if (ten == 0) {
    right_spacing += 3;
  }
  for (int i = 0; i < FONT_HEIGHT; i++) {
    uint16_t b = sign_fonts[sign][i];
    b <<= 1; // character spacing
    if (ten > 0) {
      b <<= FONT_WIDTH;
      b += number_fonts[ten][i];
      b <<= 1; // character spacing
    }
    b <<= FONT_WIDTH;
    b += number_fonts[one][i];
    b <<= right_spacing;
    _bitmap[row++] = b;
  }
  _bitmap[7] = 0;
  _setMatrix(_bitmap);
  _blink_flag = 0;
}

/**
 * Copy _bitmap to _bitmap0.
 * _bitmap is a private member and current displayed.
 */
void LEDMatrixDigit::save0() {
  for (int i = 0; i < MATRIX_HEIGHT; i++) {
    _bitmap0[i] = _bitmap[i];
  }
}

/**
 * Copy _bitmap0 to _bitmap.
 * _bitmap is a private member and current displayed.
 */
void LEDMatrixDigit::load0() {
  for (int i = 0; i < MATRIX_HEIGHT; i++) {
    _bitmap[i] = _bitmap0[i];
  }
}

/**
 * Display _bitmap0 on LED matrix.
 */
void LEDMatrixDigit::print0() {
  _setMatrix(_bitmap0);
}

/**
 * Turn on or off the LED specified by 'x' and 'y'.
 * Toggle on or off based on the 'value' of true or false
 *
 * @param 'x' represents the coordinate with the leftmost point as 0.
 * @param 'y' represents the coordinate with the top point as 0.
 * @param 'value' of true or false represents to turn on or off.
 */
void LEDMatrixDigit::setDot(int x, int y, bool value) {
  if (x < 0 || x >= MATRIX_WIDTH) return;
  if (y < 0 || y >= MATRIX_HEIGHT) return;
  int bitmap_x = MATRIX_WIDTH - x - 1;
  int bitmap_y = y;
  if (value) {
    _bitmap[bitmap_y] |= (1 << bitmap_x);
  }
  else {
    _bitmap[bitmap_y] &= ~(1 << bitmap_x);
  }
  _setMatrix(_bitmap);
}

/**
 * Blink LED matrix.
 * Each time this method is called, the LED matrix alternates between truning on and off.
 * The effect applies only to the LED that is currently LIT.
 */
int LEDMatrixDigit::blink() {
  if (_blink_flag == 0) {
    save0();
    _blink_flag = 1;
    _setMatrix(_bitmap_blank);
    load0();
    return 1;
  }
  else {
    _blink_flag = 0;
    _setMatrix(_bitmap);
    return 0;
  }
}

/**
 * Blink LED matrix with invert.
 * Each time this method is called, the LED matrix alternates between truning on and off.
 * The effect applies to the ALL LED.
 */
int LEDMatrixDigit::blinkInvert() {
  if (_blink_flag == 0) {
    save0();
    _blink_flag = 1;
    _setMatrixInvert(_bitmap);
    load0();
    return 1;
  }
  else {
    _blink_flag = 0;
    _setMatrix(_bitmap);
    return 0;
  }
}

/**
 * Turn off all LED.
 */
void LEDMatrixDigit::clearMatrix() {
  _setMatrix(_bitmap_blank);
}

/**
 * Turn off all LED with amination.
 */
void LEDMatrixDigit::animatedClear() {
  for (int y = 0; y < MATRIX_HEIGHT; y++) {
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      setDot(x, y, true);
      delay(5);
    }
  }
  for (int y = MATRIX_HEIGHT-1; y >= 0; y--) {
    for (int x = MATRIX_WIDTH-1; x >= 0; x--) {
      setDot(x, y, false);
      delay(5);
    }
  }
}

/**
 * Display from a bitmap data.
 *
 * @param 'bitmap' in MATRIX_WIDTH and MATRIX_HEIGHT.
 */
void LEDMatrixDigit::loadBitmap(uint16_t bitmap[MATRIX_HEIGHT]) {
  _setMatrix(bitmap);
}

/**
 * test methods of LEDMatrixDigit
 */
void LEDMatrixDigit::testPattern()
{
  Serial.println("DEBUG: starting testPattern");
  // Shapes of number
  // Arrengement numbers
  // WiFiDigit::print()
  for (int i = -99; i < 99; i++) {
    print(i);
    delay(100);
  }

  // WiFiDigit::clearMatrix()
  clearMatrix();
  delay(1000);

  // WiFiDigit::loadBitmap()
  // WiFiDigit::blink()
  // WiFiDigit::blinkInver()
  uint16_t invader30[MATRIX_HEIGHT] = {
    0b00011000,
    0b00111100,
    0b01111110,
    0b11011011,
    0b11111111,
    0b01011010,
    0b10000001,
    0b01000010,
  };
  for (int i = 0; i < 8; i++) {
    uint16_t bm[MATRIX_HEIGHT];
    for (int y = 0; y < 7; y++) {
      bm[y] = invader30[y] >> (8-i);
    }
    loadBitmap(bm);
    delay(300);
  }
  for (int i = 0; i < 3; i++) {
    loadBitmap(invader30);
    for (int y = 0; y < 8; y++) {
      invader30[y] <<= 1;
    }
    delay(300);
  }
  for (int i = 0; i <20; i++) {
    blink();
    delay(100);
  }
  for (int i = 0; i <20; i++) {
    blinkInvert();
    delay(100);
  }
  for (int i = 0; i < 10; i++) {
    loadBitmap(invader30);
    for (int y = 0; y < 8; y++) {
      invader30[y] <<= 1;
    }
    delay(300);
  }
  delay(1000);

  // WiFiDigit::upsideDown()
  // WiFiDigit::animatedClear() (It also calls WiFiDigit::setDot())
  uint16_t invader20[MATRIX_HEIGHT] = {
    0b00100000100,
    0b00010001000,
    0b00111111100,
    0b01101110110,
    0b11111111111,
    0b10111111101,
    0b10100000101,
    0b00011011000,
  };
  loadBitmap(invader20);
  delay(1000);
  upsideDown(true);
  loadBitmap(invader20);
  delay(1000);
  upsideDown(false);
  loadBitmap(invader20);
  delay(1000);
  animatedClear();

  // WiFiDigit::save0()
  // WiFiDigit::print0()
  uint16_t invader10a[MATRIX_HEIGHT] = {
    0b000011110000,
    0b011111111110,
    0b111111111111,
    0b111001100111,
    0b111111111111,
    0b000110011000,
    0b001100001100,
    0b110001100011,
  };
  uint16_t invader10b[MATRIX_HEIGHT] = {
    0b000011110000,
    0b011111111110,
    0b111111111111,
    0b111001100111,
    0b111111111111,
    0b001110011100,
    0b011001100110,
    0b001100001100,
  };
  loadBitmap(invader10a);
  save0();
  delay(1000);
  for (int i = 0; i < 5; i++) {
    loadBitmap(invader10b);
    delay(1000);
    print0();
    delay(1000);
  }
}

/**
 * Display upside down.
 *
 * @param Display upside down if 'upside_down' is true.
 */
void LEDMatrixDigit::upsideDown(bool upside_down) {
  _upside_down = upside_down;  
}

/**
 * Convert bitmap to frame.
 * 
 * @param Convert private member '_bitmap' and store into 'frame'.
 */
void LEDMatrixDigit::_bitmap2frame(uint32_t frame[FRAME_HEIGHT]) {
  uint32_t b = 0;
  // 12 + 12 + 8
  b = _bitmap[0] & 0x0fff;
  b <<= 12;
  b += _bitmap[1] & 0x0fff;
  b <<= 8;
  b += (_bitmap[2] >> 4) & 0x00ff;
  frame[0] = b;

  // 4 + 12 + 12 + 4
  b = _bitmap[2] & 0x000f;
  b <<= 12;
  b += _bitmap[3] & 0x0fff;
  b <<= 12;
  b += _bitmap[4] & 0x0fff;
  b <<= 4;
  b += (_bitmap[5] >> 8) & 0x000f;
  frame[1] = b;

  // 8 + 12 + 12
  b = _bitmap[5] & 0x00ff;
  b <<= 12;
  b += _bitmap[6] & 0x0fff;
  b <<= 12;
  b += _bitmap[7] & 0x0fff;
  frame[2] = b;
}

/**
 * Set bitmap to LED matrix.
 * 
 * @param bitmap by MATRIX_WIDTH and MATRIX_HEIGHT
 *
 * 'bitmap' is copied to '_bitmap' private member.
*/
void LEDMatrixDigit::_setMatrix(uint16_t bitmap[MATRIX_HEIGHT])
{
  if (_upside_down) {
    _reverseBits(bitmap, _bitmap);
  }
  else {
    for (int i = 0; i < MATRIX_HEIGHT; i++) {
      _bitmap[i] = bitmap[i];
    }
  }
  uint32_t frame[FRAME_HEIGHT];
  _bitmap2frame(frame);
  _matrix.loadFrame(frame);
}

/**
 * Set bitmap to LED matrix with inverting all bits.
 * 
 * @param bitmap by MATRIX_WIDTH and MATRIX_HEIGHT
 *
 * Inverted 'bitmap' is copied to '_bitmap' private member.
*/
void LEDMatrixDigit::_setMatrixInvert(uint16_t bitmap[MATRIX_HEIGHT])
{
  if (_upside_down) {
    _reverseBits(bitmap, _bitmap);
  }
  else {
    for (int i = 0; i < MATRIX_HEIGHT; i++) {
      _bitmap[i] = bitmap[i];
    }
  }
  uint32_t frame[FRAME_HEIGHT];
  _bitmap2frame(frame);
  for (int i = 0; i < FRAME_HEIGHT; i++) {
    frame[i] = ~frame[i]; // invert bits
  }
  _matrix.loadFrame(frame);
}

/**
 * Reverse bits of LED matrix size 12x8 bits.
 *
 * @param x is 12x8 bit matrix.
 * @param y is reversed matrix.
*/
void LEDMatrixDigit::_reverseBits(uint16_t x[MATRIX_HEIGHT], uint16_t r[MATRIX_HEIGHT]) {
  for (int i = 0; i < MATRIX_HEIGHT; i++) {
    r[i] = _reverseBits(x[MATRIX_HEIGHT - i - 1] & 0x0fff);
  }
}

/**
 * Reverse bits of a given 12 bits unsigned integer.
 *
 * @param x is 12 bits.
 * @return reversed 12 bits.
*/
uint16_t LEDMatrixDigit::_reverseBits(uint16_t x) {
  uint16_t r = x;

  r = (r & 0x5555) << 1 | (r & 0xaaaa) >> 1;
  r = (r & 0x3333) << 2 | (r & 0xcccc) >> 2;
  r = (r & 0x0f0f) << 4 | (r & 0xf0f0) >> 4;
  r = (r & 0x00ff) << 8 | (r & 0xff00) >> 8;

  return r >> 4;
}

#endif // LEDMatrixDigit_h
