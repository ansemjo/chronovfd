#ifndef __HVSHIFT_H_
#define __HVSHIFT_H_

// bitbang spi pins to hv5812p
#if defined(__PROTOCLOCK__)
  #define HV_CLOCK  7
  #define HV_DATA   9
  #define HV_STROBE 8
#else
  #error "no HV5812P pinout defined for this build"
#endif

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