#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// FNV-1a 64-bit. Nicht crypto-sicher, reicht aber, um voneinander unabhängige
// Builds (mit unterschiedlichem PROJECT_KEY) zuverlässig auseinanderzuhalten.
inline uint64_t fnv1a64(const uint8_t* data, size_t len, uint64_t seed) {
  uint64_t h = seed;
  for (size_t i = 0; i < len; i++) {
    h ^= data[i];
    h *= 0x100000001b3ULL;
  }
  return h;
}

constexpr uint64_t FNV_OFFSET_BASIS = 0xcbf29ce484222325ULL;

// 32-Bit-Magic, das jedes Sync-Paket trägt. Identifiziert "dieses Projekt mit
// diesem Key". Andere Builds mit anderem Key haben ein anderes Magic.
inline uint32_t deriveSyncMagic(const char* key) {
  uint64_t h = fnv1a64((const uint8_t*)key, strlen(key), FNV_OFFSET_BASIS);
  return (uint32_t)(h ^ (h >> 32));
}

// Auth-Tag für ein Paket mit gegebenem color-Wert. key wird beidseitig
// vor und nach dem color-Byte einmischt, damit ein Tag aus einem Paket
// nicht auf andere color-Werte übertragen werden kann, ohne den Key zu kennen.
inline uint32_t syncTagFor(const char* key, uint8_t color) {
  uint64_t h = fnv1a64((const uint8_t*)key, strlen(key), FNV_OFFSET_BASIS);
  h = fnv1a64(&color, 1, h);
  h = fnv1a64((const uint8_t*)key, strlen(key), h);
  return (uint32_t)(h ^ (h >> 32));
}
