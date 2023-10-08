#include <stddef.h>
#include <string.h>
#include <stdio.h>

#define autofclose __attribute__((cleanup(cleanup_fclose)))

static inline void cleanup_fclose(FILE** stream) {
  if (*stream)
    fclose(*stream);
}

typedef struct conf {
  char* bootfs;
  char* bootfstype;
  char* fs;
  char* fstype;
} conf;

static inline char* read_conf(const char* file) {
  autofclose FILE* f = fopen(file, "r");
  char* conf = NULL;
  size_t len;

  if (!f)
    return NULL;

  /* Note that /proc/cmdline will not end in a newline, so getline
   * will fail unelss we provide a length.
   */
  if (getline(&conf, &len, f) < 0)
    return NULL; 
  
  /* ... but the length will be the size of the malloc buffer, not
   * strlen().  Fix that.
   */
  len = strlen(conf);
  
  if (conf[len - 1] == '\n')
    conf[len - 1] = '\0';

  return conf;
}

int main(void) {
  read_conf("a.txt");
  return 0;
}

