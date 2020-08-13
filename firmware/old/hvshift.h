#ifndef __HVSHIFT_H_
#define __HVSHIFT_H_

SPIClass HVSPI(HSPI);

class HV {
  public:
    static void begin();
    static void write(uint16_t data);
    static void text(const char *str);
    static void raw(const uint16_t *buf);
    static void clear();
};

extern HV hv;

#endif
