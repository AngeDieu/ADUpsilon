#ifndef WEB_H
#define WEB_H
#include <emscripten.h>
#include <ion/events.h>
#define LCD_WIDTH_PX 320
#define LCD_HEIGHT_PX 240
#define C_WHITE 0xffff
#define C_BLACK 0
#define C_BLUE (31)
#define C_GREEN (63<<5)
#define C_RED (31<<11)

#define KEY_LEFT 0
#define KEY_UP 1
#define KEY_DOWN 2
#define KEY_RIGHT 3
#define KEY_CTRL_OK 4
#define KEY_EXIT 5
#define KEY_PLUS  45
#define KEY_MINUS 46
#define KEY_EXE 30052
#define KEY_DEL 17
#define KEY_DIV 40
#define KEY_MENU 6
#define KEY_OPTN 16
#define KEY_POW 7
#define KEY_STORE 23
#define KEY_ANS 30051
#define KEY_1 '1'
#define KEY_9 '9'

int os_draw_string_small(int x,int y,int c,int bg,const char * s,int fake);
int os_draw_string(int x,int y,int c,int bg,const char * s,int fake);
void os_fill_rect(int x,int y,int w,int h,int c);

void os_wait_1ms(int ms);

void sync_screen();
inline void dupdate(){
  sync_screen();
}

int do_getkey();

inline void dtext(int x,int y,int fg,const char * s){
  os_draw_string_small(x,y,fg,C_WHITE,s,false);
}

inline void dclear(int c){
  os_fill_rect(0,0,LCD_WIDTH_PX,LCD_HEIGHT_PX,c);
}

inline void console_log(const char * s){
  EM_ASM({
      var value = UTF8ToString($0);
      console.log(value);
    },s);
}

#endif
