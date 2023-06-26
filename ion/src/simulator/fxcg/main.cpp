#include "main.h"
#include "display.h"
#include "platform.h"

#include <gint/display-cg.h>
#include <gint/gint.h>
#include <gint/display.h>
#include <gint/keyboard.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include <ion.h>
#include <ion/events.h>
#include "menuHandler.h"
extern "C" void gint_osmenu_native(void);

static const char * storage_name="nwstore.nws";

int save_state(const char * fname); // apps/home/controller.cpp

extern "C" {
  extern const int prizm_heap_size;
  const int prizm_heap_size=192*1024;
  __attribute__((aligned(4))) char prizm_heap[prizm_heap_size];

#define GetOSVersion() ((char *)0x80020020)
  int calculator=0; // -1 means OS not checked, 0 unknown, 1 cg50 or 90, 2 emu 50 or 90, 3 other

  int main() {
    /* Allow the user to use memory past the 2 MB line on tested OS versions */
    char const *osv = GetOSVersion();
    if (!strncmp(osv, "03.", 3) && osv[3] <= '8'){ // 3.80 or earlier
      char buf[256];
      strncpy(buf,osv,8);
      buf[8]=0;
      // { print_msg12("OS ok version",buf); int key; ck_getkey(&key); }
    }
    else {
      char buf1[10],buf[256];
      // sprintf(buf,"%i",availram);
      strncpy(buf1,osv,8);
      buf1[8]=0;
      sprintf(buf,"OS %s not checked",buf1);
      calculator=-1;
      dclear(C_WHITE);
      dtext(1,10,C_BLACK,buf);
      dtext(1,27,C_BLACK,"F6: continue anyway");
      dupdate();
      int key=getkey().key;
      if (key!=KEY_F6){
	Ion::Simulator::FXCGMenuHandler::openMenu();
      }
    }
    bool is_emulator = *(volatile uint32_t *)0xff000044 == 0x00000000;
    uint32_t stack;
    __asm__("mov r15, %0" : "=r"(stack));
    bool prizmoremu=stack<0x8c000000;
    /* À appeler une seule fois au début de l'exécution */
    if (prizmoremu) {
      /* Prizm ou émulateur Graph 90+E */
      calculator=is_emulator?2:3;
    }
    else {
      calculator=1; // 0x8c200000, size 0x300000 is free
#if 0
      dclear(C_WHITE);
      dtext(1,10,C_BLACK,"Calculator has 3M RAM");
      dupdate();
      int key=getkey().key;
#endif
    }

    Ion::Simulator::Main::init();
    ion_main(0, NULL);
    Ion::Simulator::Main::quit();
    
    return 0;
  }
}

namespace Ion {
namespace Simulator {
namespace Main {

static bool sNeedsRefresh = false;

void init() {
  Ion::Simulator::Display::init();
  setNeedsRefresh();
}

void setNeedsRefresh() {
  sNeedsRefresh = true;
}

void refresh() {
  if (!sNeedsRefresh) {
    return;
  }

  Display::draw();

  sNeedsRefresh = false;
}

void quit() {
  Ion::Simulator::Display::quit();
}

void EnableStatusArea(int opt) {
  __asm__ __volatile__ (
    ".align 2 \n\t"
    "mov.l 2f, r2 \n\t"
    "mov.l 1f, r0 \n\t"
    "jmp @r2 \n\t"
    "nop \n\t"
    ".align 2 \n\t"
    "1: \n\t"
    ".long 0x02B7 \n\t"
    ".align 4 \n\t"
    "2: \n\t"
    ".long 0x80020070 \n\t"
  );
}

extern "C" void *__GetVRAMAddress(void);

uint8_t xyram_backup[16 * 1024];
uint8_t ilram_backup[4 * 1024];

void worldSwitchHandler(void (*worldSwitchFunction)(), bool prepareVRAM) {
  // Back up XYRAM
  uint8_t* xyram = (uint8_t*) 0xe500e000;
  memcpy(xyram_backup, xyram, 16 * 1024);
  // Back up ILRAM
  uint8_t* ilram = (uint8_t*) 0xe5200000;
  memcpy(ilram_backup, ilram, 4 * 1024);

  if (prepareVRAM) {
    // Copying the screen to the OS's VRAM avoids a flicker when powering on
    uint16_t* dst = (uint16_t *) __GetVRAMAddress();
    uint16_t* src = gint_vram + 6;

    for (int y = 0; y < 216; y++, dst += 384, src += 396) {
      for (int x = 0; x < 384; x++) {
        dst[x] = src[x];
      }
    }

    // Disable the status area
    EnableStatusArea(3);
  }

  worldSwitchFunction();

  // Restore XYRAM
  memcpy(xyram, xyram_backup, 16 * 1024);
  // Restore ILRAM
  memcpy(ilram, ilram_backup, 4 * 1024);
}

void runPowerOffSafe(void (*powerOffSafeFunction)(), bool prepareVRAM) {

  gint_world_switch(GINT_CALL(save_state,storage_name));
  //gint_world_switch(GINT_CALL(gint_osmenu_native));

  gint_world_switch(GINT_CALL(worldSwitchHandler, powerOffSafeFunction, prepareVRAM));
}

}
}
}
