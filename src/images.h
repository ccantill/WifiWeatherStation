#include <Arduino.h>

#define status_width 7
#define status_height 7

const uint8_t status_wifi_full_bits[] PROGMEM = {
  0x1C, 0x22, 0x41, 0x1C, 0x22, 0x00, 0x08, };

const uint8_t status_wifi_medium_bits[] PROGMEM = {
  0x00, 0x00, 0x00, 0x1C, 0x22, 0x00, 0x08, };

const uint8_t status_wifi_poor_bits[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, };

const uint8_t status_wifi_connecting_bits[] PROGMEM = {
  0x3E, 0x2A, 0x14, 0x08, 0x14, 0x22, 0x3E, };

const uint8_t status_wifi_deepsleep_bits[] PROGMEM = {
  0x3E, 0x20, 0x10, 0x08, 0x04, 0x02, 0x3E, };