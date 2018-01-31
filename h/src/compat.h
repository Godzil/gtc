#ifdef DOORS
 #define _CALCULATOR _ram_call(0,const unsigned char*)
#endif

#if defined(DOORS) && !defined(_ONE_CALC_ONLY)
 #define LCD_WIDTH _ram_call(1,unsigned long)
 #define LCD_HEIGHT _ram_call(2,unsigned long)
 #define LCD_LINE_BYTES _ram_call(4,unsigned long)
 #define KEY_LEFT _ram_call(5,unsigned long)
 #define KEY_RIGHT _ram_call(6,unsigned long)
 #define KEY_UP _ram_call(7,unsigned long)
 #define KEY_DOWN _ram_call(8,unsigned long)
 #define KEY_UPRIGHT _ram_call(9,unsigned long)
 #define KEY_DOWNLEFT _ram_call(A,unsigned long)
 #define KEY_DIAMOND _ram_call(B,unsigned long)
 #define KEY_SHIFT _ram_call(D,unsigned long)
 #define CALCULATOR (_CALCULATOR[0])
 #define HW_VERSION (_CALCULATOR[1])
#else
 #if defined (_TI89_ONLY)
  #define ROM_base ((void*)(((unsigned long)__jmp_tbl)&0xE00000))
  #define CALCULATOR 0
 #else
#if defined (_TI92PLUS_ONLY)
  #define ROM_base ((void*)0x400000)
  #define CALCULATOR 1
 #else
#if defined (_V200_ONLY)
  #define ROM_base ((void*)(((unsigned long)__jmp_tbl)&0xE00000))
  #define CALCULATOR 3
 #else
  #ifdef DOORS
   #define ROM_base _ram_call (3, const void*)
  #else
   #define ROM_base ((void*)(((unsigned long)__jmp_tbl)&0xE00000))
  #endif
  #ifdef NO_CALC_DETECT
   #ifdef USE_V200
    #define CALCULATOR (ROM_base==(void*)0x400000?1:(((unsigned char*)(_rom_call_addr(2F)))[2]>=200?3:0))
   #else
    #define CALCULATOR (ROM_base==(void*)0x400000)
   #endif
  #else
   extern const short __calculator;
   #ifdef USE_TI89
    #define CALCULATOR (__calculator)
   #else
    #define CALCULATOR (__calculator==3?3:1)
   #endif
  #endif
#endif
#endif
 #endif
 #define KEY_DIAMOND (CALCULATOR?8192U:16384U)
 #define KEY_DOWN (CALCULATOR?344:340)
 #define KEY_DOWNLEFT (CALCULATOR?345:342)
 #define KEY_LEFT (CALCULATOR?337:338)
 #define KEY_OFF2 (CALCULATOR?8459U:16651U)
 #define KEY_RIGHT (CALCULATOR?340:344)
 #define KEY_SHIFT (CALCULATOR?16384U:8192U)
 #define KEY_UP (CALCULATOR?338:337)
 #define KEY_UPRIGHT (CALCULATOR?342:345)
 #define LCD_HEIGHT (CALCULATOR?128:100)
 #define LCD_LINE_BYTES (CALCULATOR?30:20)
 #define LCD_WIDTH (CALCULATOR?240:160)
#endif
