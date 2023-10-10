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

#ifdef __cplusplus
#define typeof decltype
#endif

#define swap(a, b)      \
  do {                  \
    typeof(a) temp = a; \
    a = b;              \
    b = temp;           \
  } while (0)

typedef struct pair {
  char* val;
  char* scoped;
} pair;

typedef struct conf {
  pair bootfs;
  pair bootfstype;
  pair fs;
  pair fstype;
} conf;

typedef struct str {
  char* c_str;
  int len;
} str;

static inline void cleanup_free_conf(conf* p) {
  free(p->bootfs.scoped);
  free(p->bootfstype.scoped);
  free(p->fs.scoped);
  free(p->fstype.scoped);
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
