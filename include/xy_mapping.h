#pragma once
#include <stdint.h>

// LED-Index für serpentinen-verdrahtete Matrix.
// y=0 ist die unterste Reihe (Boden), y=ny-1 ist die oberste (Spitze).
// Physische Reihennummerierung läuft von oben (physY=0) nach unten (physY=ny-1);
// ungeradzahlige physische Reihen sind in X-Richtung gespiegelt verdrahtet.
inline uint16_t xyMap(uint8_t x, uint8_t y, uint8_t nx, uint8_t ny) {
  if (x >= nx || y >= ny) return 0;
  uint8_t physY = (ny - 1) - y;
  if (physY & 1) {
    return (physY * nx) + (nx - 1 - x);
  } else {
    return (physY * nx) + x;
  }
}
