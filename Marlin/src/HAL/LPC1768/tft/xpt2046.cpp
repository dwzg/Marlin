/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifdef TARGET_LPC1768

#include "../../../inc/MarlinConfig.h"

#if HAS_TFT_XPT2046 || HAS_RES_TOUCH_BUTTONS

#include "xpt2046.h"
#include <SPI.h>

uint16_t delta(uint16_t a, uint16_t b) { return a > b ? a - b : b - a; }

#if ENABLED(TOUCH_BUTTONS_HW_SPI)
  #include <SPI.h>

  SPIClass XPT2046::SPIx(TOUCH_BUTTONS_HW_SPI_DEVICE);

  static void touch_spi_init(uint8_t spiRate) {
    XPT2046::SPIx.setModule(TOUCH_BUTTONS_HW_SPI_DEVICE);
    XPT2046::SPIx.setClock(SPI_CLOCK_DIV128);
    XPT2046::SPIx.setBitOrder(MSBFIRST);
    XPT2046::SPIx.setDataMode(SPI_MODE0);
    XPT2046::SPIx.setDataSize(DATA_SIZE_8BIT);
  }
#endif

void XPT2046::init() {
  #if DISABLED(TOUCH_BUTTONS_HW_SPI)
    SET_INPUT(TOUCH_MISO_PIN);
    SET_OUTPUT(TOUCH_MOSI_PIN);
    SET_OUTPUT(TOUCH_SCK_PIN);
  #endif
  OUT_WRITE(TOUCH_CS_PIN, HIGH);

  #if PIN_EXISTS(TOUCH_INT)
    // Optional Pendrive interrupt pin
    SET_INPUT(TOUCH_INT_PIN);
  #endif

  TERN_(TOUCH_BUTTONS_HW_SPI, touch_spi_init(SPI_SPEED_6));

  // Read once to enable pendrive status pin
  getRawData(XPT2046_X);
}

bool XPT2046::isTouched() {
  return isBusy() ? false : (
    #if PIN_EXISTS(TOUCH_INT)
      READ(TOUCH_INT_PIN) != HIGH
    #else
      getRawData(XPT2046_Z1) >= XPT2046_Z1_THRESHOLD
    #endif
  );
}

bool XPT2046::getRawPoint(int16_t * const x, int16_t * const y) {
  if (isBusy() || !isTouched()) return false;
  *x = getRawData(XPT2046_X);
  *y = getRawData(XPT2046_Y);
  return true;
}

uint16_t XPT2046::getRawData(const XPTCoordinate coordinate) {
  uint16_t data[3];

  dataTransferBegin();
  TERN_(TOUCH_BUTTONS_HW_SPI, SPIx.begin());

  for (uint16_t i = 0; i < 3 ; i++) {
    IO(coordinate);
    data[i] = (IO() << 4) | (IO() >> 4);
  }

  TERN_(TOUCH_BUTTONS_HW_SPI, SPIx.end());
  dataTransferEnd();

  uint16_t delta01 = delta(data[0], data[1]),
           delta02 = delta(data[0], data[2]),
           delta12 = delta(data[1], data[2]);

  if (delta01 > delta02 || delta01 > delta12)
    data[delta02 > delta12 ? 0 : 1] = data[2];

  return (data[0] + data[1]) >> 1;
}

uint16_t XPT2046::IO(uint16_t data) {
  return TERN(TOUCH_BUTTONS_HW_SPI, hardwareIO, softwareIO)(data);
}

extern uint8_t spiTransfer(uint8_t b);

#if ENABLED(TOUCH_BUTTONS_HW_SPI)
  uint16_t XPT2046::hardwareIO(uint16_t data) {
    return SPIx.transfer(data & 0xFF);
  }
#endif

uint16_t XPT2046::softwareIO(uint16_t data) {
  uint16_t result = 0;

  for (uint8_t j = 0x80; j; j >>= 1) {
    WRITE(TOUCH_SCK_PIN, LOW);
    WRITE(TOUCH_MOSI_PIN, data & j ? HIGH : LOW);
    if (READ(TOUCH_MISO_PIN)) result |= j;
    WRITE(TOUCH_SCK_PIN, HIGH);
  }
  WRITE(TOUCH_SCK_PIN, LOW);

  return result;
}

#endif // HAS_TFT_XPT2046 || HAS_RES_TOUCH_BUTTONS
#endif // TARGET_LPC1768
