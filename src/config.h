#ifndef CONFIG_H
#define CONFIG_H
#endif

typedef struct CALLBACK {
  struct CALLBACK (* func)(cbuf_t* buf_in, cbuf_t* buf_out, void* state);
  void* debug_info; // just in case
} callback_t ;
