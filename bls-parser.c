#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define autofree __attribute__((cleanup(cleanup_free)))
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

static inline void cleanup_free(void* p) {
  free(*(void**)p);
}

static inline void cleanup_fclose(FILE** stream) {
  if (*stream)
    fclose(*stream);
}

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

static inline char* read_conf(const char* file, conf* conf) {
  autofclose FILE* f = fopen(file, "r");
  autofree char* line = NULL;
  size_t len;

  if (!f)
    return NULL;

  /* Note that /proc/cmdline will not end in a newline, so getline
   * will fail unelss we provide a length.
   */
  while (getline(&line, &len, f) >= 0) {
     if (!strcmp(line, "bootfs"))
       SWAP(conf->bootfs_scoped, line);
     else if (!strcmp(line, "bootfstype"))
       SWAP(conf->bootfstype_scoped, line);
     else if (!strcmp(line, "fs"))
       SWAP(conf->fs_scoped, line);
     else if (!strcmp(line, "fstype"))
       SWAP(conf->fstype_scoped, line);
  }
  
  return NULL;
}

int main(void) {
  conf conf;
  read_conf("a.txt", &conf);
  return 0;
}

