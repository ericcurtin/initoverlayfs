#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define autofree __attribute__((cleanup(cleanup_free)))
#define autofreeconf __attribute__((cleanup(cleanup_freeconf)))
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

static inline void cleanup_freeconf(conf* p) {
  free(p->bootfs_scoped);
  free(p->bootfstype_scoped);
  free(p->fs_scoped);
  free(p->fstype_scoped);
}

#define BOOTFS_LEN sizeof("bootfs") - 1
#define BOOTFSTYPE_LEN sizeof("bootfstype") - 1
#define FS_LEN sizeof("fs") - 1
#define FSTYPE_LEN sizeof("fstype") - 1

static inline bool is_line_key(const char* line,
                               const size_t line_len,
                               const char* key,
                               const size_t key_len) {
  return line_len > key_len && isspace(line[key_len]) &&
         !strncmp(line, key, key_len);
}

static inline void set_conf_val(char** conf_scoped,
                                char** conf,
                                char** line,
                                const size_t line_len,
                                const size_t key_len) {
  int i;
  for (i = key_len; isspace((*line)[i]); ++i)
    ;
  *conf = *line + i;

  for (i = line_len; isspace((*line)[i]); --i)
    ;
  (*line)[i - 1] = 0;

  SWAP(*conf_scoped, *line);
}

static inline char* read_conf(const char* file, conf* conf) {
  autofclose FILE* f = fopen(file, "r");
  autofree char* line = NULL;
  size_t len_alloc = 0;
  int len;

  if (!f)
    return NULL;

  /* Note that /proc/cmdline will not end in a newline, so getline
   * will fail unelss we provide a length.
   */
  while ((len = getline(&line, &len_alloc, f)) != -1) {
    if (is_line_key(line, len, "bootfs", BOOTFS_LEN))
      set_conf_val(&conf->bootfs_scoped, &conf->bootfs, &line, len, BOOTFS_LEN);
    else if (is_line_key(line, len, "bootfstype", BOOTFSTYPE_LEN))
      set_conf_val(&conf->bootfstype_scoped, &conf->bootfstype, &line, len,
                   BOOTFSTYPE_LEN);
    else if (is_line_key(line, len, "fs", FS_LEN))
      set_conf_val(&conf->fs_scoped, &conf->fs, &line, len, FS_LEN);
    else if (is_line_key(line, len, "fstype", FSTYPE_LEN))
      set_conf_val(&conf->fstype_scoped, &conf->fstype, &line, len, FSTYPE_LEN);
  }

  return NULL;
}

int main(void) {
  autofreeconf conf conf = {.bootfs = 0,
                            .bootfstype = 0,
                            .fs = 0,
                            .fstype = 0,
                            .bootfs_scoped = 0,
                            .bootfstype_scoped = 0,
                            .fs_scoped = 0,
                            .fstype_scoped = 0};
  read_conf("a.txt", &conf);
  printf(
      "bootfs: '%s' bootfstype: '%s' fs: '%s' fstype: '%s' bootfs_scoped: '%s' "
      "bootfstype_scoped: '%s' fs_scoped: '%s' fstype_scoped: '%s'\n",
      conf.bootfs ?: "", conf.bootfstype ?: "", conf.fs ?: "",
      conf.fstype ?: "", conf.bootfs_scoped ?: "", conf.bootfstype_scoped ?: "",
      conf.fs_scoped ?: "", conf.fstype_scoped ?: "");
  return 0;
}
