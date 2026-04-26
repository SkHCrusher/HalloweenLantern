#include <unity.h>
#include <cstdio>
#include "xy_mapping.h"
#include "sync_keys.h"

void setUp(void) {}
void tearDown(void) {}

// ===== XY MAPPING =====

void test_xy_bottom_left_maps_to_last_serpentine_row(void) {
  // 10x12 Matrix. y=0 ist die unterste Reihe (Boden), physY = 11 (ungerade,
  // also gespiegelt). x=0 → physischer Index 11*10 + (10-1-0) = 119.
  TEST_ASSERT_EQUAL_UINT16(119, xyMap(0, 0, 10, 12));
}

void test_xy_top_right_maps_to_first_row(void) {
  // y=11 ist Spitze. physY = 0 (gerade, nicht gespiegelt). x=9 → 0*10 + 9 = 9.
  TEST_ASSERT_EQUAL_UINT16(9, xyMap(9, 11, 10, 12));
}

void test_xy_serpentine_alternates(void) {
  // physY=0 (gerade) und physY=1 (ungerade) müssen entgegengesetzt verlaufen.
  // y=11 → physY=0 (direkt), y=10 → physY=1 (gespiegelt)
  TEST_ASSERT_EQUAL_UINT16(0, xyMap(0, 11, 10, 12));   // physY=0, x=0
  TEST_ASSERT_EQUAL_UINT16(19, xyMap(0, 10, 10, 12));  // physY=1, x=0 → 1*10+9 = 19
  TEST_ASSERT_EQUAL_UINT16(10, xyMap(9, 10, 10, 12));  // physY=1, x=9 → 1*10+0 = 10
}

void test_xy_out_of_bounds_returns_zero(void) {
  TEST_ASSERT_EQUAL_UINT16(0, xyMap(10, 0, 10, 12));
  TEST_ASSERT_EQUAL_UINT16(0, xyMap(0, 12, 10, 12));
  TEST_ASSERT_EQUAL_UINT16(0, xyMap(255, 255, 10, 12));
}

// ===== FNV-1a =====

void test_fnv1a_empty_returns_offset_basis(void) {
  TEST_ASSERT_EQUAL_HEX64(0xcbf29ce484222325ULL,
                          fnv1a64((const uint8_t*)"", 0, FNV_OFFSET_BASIS));
}

void test_fnv1a_known_vector_a(void) {
  // Standard FNV-1a 64-Bit Test-Vektor: "a" → 0xaf63dc4c8601ec8c
  TEST_ASSERT_EQUAL_HEX64(0xaf63dc4c8601ec8cULL,
                          fnv1a64((const uint8_t*)"a", 1, FNV_OFFSET_BASIS));
}

void test_fnv1a_known_vector_foobar(void) {
  // Standard FNV-1a 64-Bit Test-Vektor: "foobar" → 0x85944171f73967e8
  TEST_ASSERT_EQUAL_HEX64(0x85944171f73967e8ULL,
                          fnv1a64((const uint8_t*)"foobar", 6, FNV_OFFSET_BASIS));
}

// ===== SYNC MAGIC =====

void test_sync_magic_is_deterministic(void) {
  TEST_ASSERT_EQUAL_UINT32(deriveSyncMagic("some-key"),
                           deriveSyncMagic("some-key"));
}

void test_sync_magic_differs_for_different_keys(void) {
  TEST_ASSERT_NOT_EQUAL(deriveSyncMagic("key-A"), deriveSyncMagic("key-B"));
}

void test_sync_magic_default_differs_from_typical_user_key(void) {
  // Sanity-Check: Fallback-Default-Key und ein realistischer User-Key
  // ergeben verschiedene Magics.
  TEST_ASSERT_NOT_EQUAL(
    deriveSyncMagic("fallback-default-set-your-own-in-platformio_local.ini"),
    deriveSyncMagic("my-actual-secret"));
}

// ===== SYNC TAG =====

void test_sync_tag_is_deterministic(void) {
  TEST_ASSERT_EQUAL_UINT32(syncTagFor("k", 3), syncTagFor("k", 3));
}

void test_sync_tag_differs_per_color(void) {
  // Verschiedene Color-Werte mit demselben Key dürfen nicht denselben Tag haben.
  for (uint8_t a = 0; a < 8; a++) {
    for (uint8_t b = a + 1; b < 8; b++) {
      char msg[64];
      snprintf(msg, sizeof(msg), "tag(k, %u) collided with tag(k, %u)", a, b);
      TEST_ASSERT_NOT_EQUAL_MESSAGE(syncTagFor("k", a), syncTagFor("k", b), msg);
    }
  }
}

void test_sync_tag_differs_per_key(void) {
  TEST_ASSERT_NOT_EQUAL(syncTagFor("key-A", 5), syncTagFor("key-B", 5));
}

// ===== RUNNER =====

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_xy_bottom_left_maps_to_last_serpentine_row);
  RUN_TEST(test_xy_top_right_maps_to_first_row);
  RUN_TEST(test_xy_serpentine_alternates);
  RUN_TEST(test_xy_out_of_bounds_returns_zero);
  RUN_TEST(test_fnv1a_empty_returns_offset_basis);
  RUN_TEST(test_fnv1a_known_vector_a);
  RUN_TEST(test_fnv1a_known_vector_foobar);
  RUN_TEST(test_sync_magic_is_deterministic);
  RUN_TEST(test_sync_magic_differs_for_different_keys);
  RUN_TEST(test_sync_magic_default_differs_from_typical_user_key);
  RUN_TEST(test_sync_tag_is_deterministic);
  RUN_TEST(test_sync_tag_differs_per_color);
  RUN_TEST(test_sync_tag_differs_per_key);
  return UNITY_END();
}
