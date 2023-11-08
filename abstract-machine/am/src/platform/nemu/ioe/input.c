#include <am.h>
#include <nemu.h>
#include <stdio.h>
#define KEYDOWN_MASK 0x8000

// void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {


//   kbd->keydown = 0;
//   kbd->keycode = AM_KEY_NONE;
// }

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
	printf("__am_input_keybrd 执行一次\n");
  uint32_t input = inl(KBD_ADDR);
  if (input != AM_KEY_NONE){
	printf("target kbd\n");
    kbd->keydown = KEYDOWN_MASK & input;
    kbd->keycode = input & (~KEYDOWN_MASK);
  }else {
    kbd->keydown = false;
    kbd->keycode = AM_KEY_NONE;
  }
}

