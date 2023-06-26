#include <ion/backlight.h>
#include "k_csdk.h"

#ifndef is_cx2
#define is_cx2 false
#endif

static unsigned oldval=0,oldval2=0;
int getRawBacklightSubLevel(){
  return 128;
  unsigned NSPIRE_CONTRAST_ADDR=is_cx2?0x90130014:0x900f0020;
  oldval=*(volatile unsigned *)NSPIRE_CONTRAST_ADDR;
  if (is_cx2){
    oldval2=*(volatile unsigned *) (NSPIRE_CONTRAST_ADDR+4);
    return oldval2>>24;
  }
  return oldval>>24;
}
void setRawBacklightSubLevel(int level){
  return;
  unsigned NSPIRE_CONTRAST_ADDR=is_cx2?0x90130014:0x900f0020;
  if (is_cx2){
    *(volatile unsigned *) (NSPIRE_CONTRAST_ADDR+4)=level<<24;//oldval2;
  }
  *(volatile unsigned *)NSPIRE_CONTRAST_ADDR=is_cx2?oldval:(level<<24);
  static volatile uint32_t *lcd_controller = (volatile uint32_t*) 0xC0000000;
  lcd_controller[6] &= ~(0b1 << 11);
  loopsleep(20);
  lcd_controller[6] &= ~ 0b1;
  
}
// END OF POWER MANAGEMENT CODE 

namespace Ion {
namespace Backlight {

uint8_t brightness() {
  return getRawBacklightSubLevel();
}

void setBrightness(uint8_t b) {
  setRawBacklightSubLevel(b);
}

void init() {
}

bool isInitialized() {
  return true;
}

void shutdown() {
}

}
}
