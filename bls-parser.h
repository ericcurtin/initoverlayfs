static inline bool is_line_key(const str* line, const str* key) {
  return line->len > key->len && isspace(line->c_str[key->len]) &&
         !strncmp(line->c_str, key->c_str, key->len);
}

static inline void set_conf(char** conf_scoped,
                            char** conf,
                            str* line,
                            const size_t key_len) {
  int i;
  for (i = key_len; isspace(line->c_str[i]); ++i)
    ;
  *conf = line->c_str + i;

  for (i = line->len; isspace(line->c_str[i]); --i)
    ;
  line->c_str[i - 1] = 0;

  SWAP(*conf_scoped, line->c_str);
}

static inline void set_conf_pick(conf* c, str* line) {
  const str bootfs_str = {.c_str = "bootfs", .len = sizeof("bootfs") - 1};
  const str bootfstype_str = {.c_str = "bootfstype",
                              .len = sizeof("bootfstype") - 1};
  const str fs_str = {.c_str = "fs", .len = sizeof("fs") - 1};
  const str fstype_str = {.c_str = "fstype", .len = sizeof("fstype") - 1};
  if (is_line_key(line, &bootfs_str))
    set_conf(&c->bootfs_scoped, &c->bootfs, line, bootfs_str.len);
  else if (is_line_key(line, &bootfstype_str))
    set_conf(&c->bootfstype_scoped, &c->bootfstype, line, bootfstype_str.len);
  else if (is_line_key(line, &fs_str))
    set_conf(&c->fs_scoped, &c->fs, line, fs_str.len);
  else if (is_line_key(line, &fstype_str))
    set_conf(&c->fstype_scoped, &c->fstype, line, fstype_str.len);
}

static inline char* read_conf(const char* file, conf* c) {
  autofclose FILE* f = fopen(file, "r");
  autofree str line = {.c_str = NULL, .len = 0};
  size_t len_alloc;

  if (!f)
    return NULL;

  while ((line.len = getline(&line.c_str, &len_alloc, f)) != -1)
    set_conf_pick(c, &line);

  return NULL;
}

#if 0
int main(void) {
  autofree_conf conf conf = {.bootfs = 0,
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
#endif
