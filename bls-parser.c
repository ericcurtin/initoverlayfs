#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

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

#define BOOTFS_LEN sizeof("bootfs") - 1
#define BOOTFSTYPE_LEN sizeof("bootfstype") - 1
#define FS_LEN sizeof("fs") - 1
#define FSTYPE_LEN sizeof("fstype") - 1

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
     if (isspace(line[BOOTFS_LEN]) && !strncmp(line, "bootfs", BOOTFS_LEN)) {
       int i;
       for (i = BOOTFS_LEN; isspace(line[i]); ++i);
       conf->bootfs = line + i;

       for (i = len; isspace(i; --i)
       SWAP(conf->bootfs_scoped, line);
     }
     else if (!strncmp(line, "bootfstype", BOOTFSTYPE_LEN))
       SWAP(conf->bootfstype_scoped, line);
     else if (!strncmp(line, "fs", FS_LEN))
       SWAP(conf->fs_scoped, line);
     else if (!strncmp(line, "fstype", FSTYPE_LEN))
       SWAP(conf->fstype_scoped, line);
  }
  
  return NULL;
}

int main(void) {
  conf conf = { 0 };
  read_conf("a.txt", &conf);
  printf("bootfs: '%s' bootfstype: '%s' fs: '%s' fstype: '%s' bootfs_scoped: '%s' bootfstype_scoped: '%s' fs_scoped: '%s' fstype_scoped: '%s'\n", conf.bootfs ?: "", conf.bootfstype ?: "", conf.fs ?: "", conf.fstype ?: "", conf.bootfs_scoped ?: "", conf.bootfstype_scoped ?: "", conf.fs_scoped ?: "", conf.fstype_scoped ?: "");
  return 0;
}

