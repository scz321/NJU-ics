#include <am.h>
#include <nemu.h>

#include <stdio.h>//这里按理说是不需要include的呀

#define SYNC_ADDR (VGACTL_ADDR + 4)

// void __am_gpu_init()
// {
// 	int i;
// 	int w = 400; // TODO: get the correct width
// 	int h = 300; // TODO: get the correct height
// 	uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;//这里会触发回调
// 	for (i = 0; i < w * h; i++)
// 		fb[i] = i;//这里本质上是对VGA的像素值进行随机写入
// 	outl(SYNC_ADDR, 1);
// }

// void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
//   *cfg = (AM_GPU_CONFIG_T) {
//     .present = true, .has_accel = false,
//     .width = 0, .height = 0,
//     .vmemsz = 0
//   };
// }
void __am_gpu_init() {
  int i;
  int w = io_read(AM_GPU_CONFIG).width;  // TODO: get the correct width
  int h = io_read(AM_GPU_CONFIG).height;  // TODO: get the correct height
  printf("w:%d, h:%d\n", w, h);
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  //这里会触发一次回调函数
  for (i = 0; i < w * h; i ++) fb[i] = i;
 // outl(SYNC_ADDR, 1);
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
//   uint32_t info = inl(VGACTL_ADDR);//这里也可能触发一次回调函数
//   uint16_t height = (uint16_t)(info & 0xFFFF);
//   uint16_t width = (uint16_t)(info >> 16);
//   *cfg = (AM_GPU_CONFIG_T) {
//     .present = true, .has_accel = false,
//     .width = width, .height = height,
//     .vmemsz = 0
//   };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  int win_weight = io_read(AM_GPU_CONFIG).width;  // TODO: get the correct width

  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  uint32_t *pi = (uint32_t *)(uintptr_t)ctl->pixels;
  // printf("(x:%d, y:%d) w=%d, h=%d", ctl->x, ctl->y, ctl->w, ctl->h);
  for (int i = 0; i < ctl->h; ++i){
    for (int j = 0; j < ctl->w; ++j){
      //fb[(ctl->y) * win_weight + i * win_weight + ctl->x + j] = pi[i * (ctl->w) + j];
      //使用outl更加稳妥，尤其是涉及到设备的时候，不对，应该说，必须使用out系列
      outl((uint32_t)(fb + ctl->y * win_weight + i * win_weight + ctl->x + j), pi[i * (ctl->w) + j]);
    }
  }

  if (ctl->sync) {
    outl(SYNC_ADDR, 1);
  }

}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
// void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
//   if (ctl->sync) {
//     outl(SYNC_ADDR, 1);
//   }
// }

// void __am_gpu_status(AM_GPU_STATUS_T *status) {
//   status->ready = true;
// }

//这个函数的作用是把*src的内容写进目标偏移量处的显存，
//uint32_t dest; void *src; int size
void __am_gpu_memcpy(AM_GPU_MEMCPY_T *params) {
	uint32_t *src = params->src;
	// dst的值应当是指定偏移量处的地址，这里是物理地址
	uint32_t *dst = (uint32_t *)(FB_ADDR + params->dest);

	for (int i = 0; i < params->size >> 2; i++, src++, dst++)
	{
		*dst = *src;
	}
	
	char *c_src = (char *)src, *c_dst = (char *)dst;

	for (int i = 0; i < (params->size & 3); i++)
	{
		c_dst[i] = c_src[i];
	}

}