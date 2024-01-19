#include <NDL.h>
#include <SDL.h>

#define keyname(k) #k,

static const char *keyname[] = {
  "NONE",
  _KEYS(keyname)
};

int SDL_PushEvent(SDL_Event *ev) {
  return 0;
}

static uint8_t key_state[sizeof(keyname) / sizeof(keyname[0])] = {0};


int SDL_PollEvent(SDL_Event *ev) {
  uint8_t type = 0, sym = 0;
  // SDL_PumpEvents();


  // if (pop(&type, &sym)){
  //printf("SDL_PollEvent\n");
  if (read_keyinfo(&type, &sym)){
    ev->type = type;
    ev->key.keysym.sym = sym;

    switch(type){
    case SDL_KEYDOWN:
      key_state[sym] = 1;
      //printf("%d Down\n", (int)sym);
      break;
    
    case SDL_KEYUP:
      key_state[sym] = 0;
      //printf("%d Up\n", (int)sym);
      break;
    }
  }else {
    return 0;
  }

  return 1;
}

int SDL_WaitEvent(SDL_Event *event) {
  return 1;
}

int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask) {
  return 0;
}

uint8_t* SDL_GetKeyState(int *numkeys) {
  return NULL;
}
