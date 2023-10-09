#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define autofree __attribute__((cleanup(cleanup_free)))
#define autofree_conf __attribute__((cleanup(cleanup_free_conf)))
#define autoclose __attribute__((cleanup(cleanup_close)))
#define autofclose __attribute__((cleanup(cleanup_fclose)))
#define autova_end __attribute__((cleanup(cleanup_va_end)))

#ifdef __cplusplus
#define typeof decltype
#endif

#define SWAP(a, b)      \
  do {                  \
    typeof(a) temp = a; \
    a = b;              \
    b = temp;           \
  } while (0)

typedef struct conf {
  char* bootfs;
  char* bootfstype;
  char* fs;
  char* fstype;
  char* bootfs_scoped;
  char* bootfstype_scoped;
  char* fs_scoped;
  char* fstype_scoped;
} conf;

typedef struct str {
  char* c_str;
  int len;
} str;

static inline void cleanup_free_conf(conf* p) {
  free(p->bootfs_scoped);
  free(p->bootfstype_scoped);
  free(p->fs_scoped);
  free(p->fstype_scoped);
}

static inline void cleanup_free(void* p) {
  free(*(void**)p);
}

static inline void cleanup_close(const int* fd) {
  if (*fd > 2)  // Greater than 2 to protect stdin, stdout and stderr
    close(*fd);
}

static inline void cleanup_fclose(FILE** stream) {
  if (*stream)
    fclose(*stream);
}

static inline void cleanup_va_end(va_list* args) {
  va_end(*args);
}
