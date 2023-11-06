
// #include <am.h>
// #include <nemu.h>



// #define TIME_BASE 0x48

// void __am_timer_init() {
	
// }


// static uint64_t read_time() {
//   uint32_t lo = *(volatile uint32_t *)(MMIO_BASE + TIME_BASE + 0);
//   uint32_t hi = *(volatile uint32_t *)(MMIO_BASE + TIME_BASE + 4);
//   uint64_t time = ((uint64_t)hi << 32) | lo;
//   return time ;
// }

// void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
//   uptime->us = read_time() - 0;
// }

// void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
//   rtc->second = 0;
//   rtc->minute = 0;
//   rtc->hour   = 0;
//   rtc->day    = 0;
//   rtc->month  = 0;
//   rtc->year   = 1900;
// }

#include <am.h>
#include <klib.h>
#include <nemu.h>

#include "../../../riscv/riscv.h"

void __am_timer_init() {}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  // printf("%d \n", RTC_ADDR);

  uptime->us = ((uint64_t)inl(RTC_ADDR + 4) << 32) | (uint64_t)inl(RTC_ADDR);

}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
  printf("当前正在执行__am_timer_rtc函数\n");
  printf("rtc->month:%d\n",rtc->day);
  //printf("%d-%d-%d %d:%d:%d GMT \n", rtc->year, rtc->month, rtc->day, rtc->hour, rtc->minute, rtc->second);
}
