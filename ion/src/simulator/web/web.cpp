#include <ion.h>
#include <kandinsky.h>
#include <escher.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include "web.h"
#include "../apps_container.h"
#include "../global_preferences.h"

void sync_screen(){
  // Refresh the display.
  // Ion::Keyboard::scan();
}

void os_wait_1ms(int ms){
  Ion::Timing::msleep(ms);
}

int do_getkey(){
  //os_wait_1ms(50);
  // copy extapp_getKey in apps/external/extapi.cpp for a more complete version
  int timeout=60000;
  Ion::Events::Event e=Ion::Events::getEvent(&timeout);
  if (e.isKeyboardEvent()){
    int res=uint8_t(e);
    switch (res){
    case 51:
      res=KEY_ANS;
      break;
    case 52:
      res=KEY_EXE;
      break;
    case 30:
      res='7';
      break;
    case 31:
      res='8';
      break;
    case 32:
      res='9';
      break;
    case 36:
      res='4';
      break;
    case 37:
      res='5';
      break;
    case 38:
      res='6';
      break;
    case 42:
      res='1';
      break;
    case 43:
      res='2';
      break;
    case 44:
      res='3';
      break;
    case 48:
      res='0';
      break;
    }
    return res;
  }
  return -1;
}

void os_fill_rect(int x,int y,int w,int h,int color){
  KDRect rect(x, y, w, h);
  Ion::Display::pushRectUniform(rect, KDColor::RGB16(color));
  sync_screen();
}

int os_draw_string(int x,int y,int fg,int bg,const char * text,int fake){
  KDPoint point(x, y);

  auto ctx = KDIonContext::sharedContext();
  ctx->setClippingRect(KDRect(0, 0, 320, fake ? 0 : 240));
  ctx->setOrigin(KDPoint(0, 0));
  point = ctx->drawString(text, point, KDFont::LargeFont, KDColor::RGB16(fg), KDColor::RGB16(bg));
  sync_screen();
  return point.x();
}

int os_draw_string_small(int x,int y,int fg,int bg,const char * text,int fake){
  KDPoint point(x, y);

  auto ctx = KDIonContext::sharedContext();
  ctx->setClippingRect(KDRect(0, 0, 320, fake ? 0 : 240));
  ctx->setOrigin(KDPoint(0, 0));
  point = ctx->drawString(text, point, KDFont::SmallFont, KDColor::RGB16(fg), KDColor::RGB16(bg));
  sync_screen();
  return point.x();
}

