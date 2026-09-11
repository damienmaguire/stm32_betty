#ifndef BMW_CRC_H
#define BMW_CRC_H
#include <stdint.h>
class BmwCrc {
public:
  static const uint8_t finalxor[12];
  static uint8_t calc(uint16_t id, const uint8_t *buf, uint8_t len, int nextmes);
};
#endif
