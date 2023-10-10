#include "pre-init.h"
#include <assert.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/loop.h>
#include <linux/magic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/vfs.h>
#include <sys/wait.h>
#include "config-parser.h"

#define fork_exec_absolute_no_wait(pid, exe, ...)            \
  do {                                                       \
    printd("fork_exec_absolute_no_wait(\"" exe "\")\n");     \
    pid = fork();                                            \
    if (pid == -1) {                                         \
      print("fail fork_exec_absolute_no_wait\n");            \
      break;                                                 \
    } else if (pid > 0) {                                    \
      printd("forked %d fork_exec_absolute_no_wait\n", pid); \
      break;                                                 \
    }                                                        \
                                                             \
    execl(exe, exe, __VA_ARGS__, (char*)NULL);               \
    exit(errno);                                             \
  } while (0)

#define fork_exec_absolute(exe, ...)                 \
  do {                                               \
    printd("fork_exec_absolute(\"" exe "\")\n");     \
    const pid_t pid = fork();                        \
    if (pid == -1) {                                 \
      print("fail exec_absolute\n");                 \
      break;                                         \
    } else if (pid > 0) {                            \
      printd("forked %d fork_exec_absolute\n", pid); \
      waitpid(pid, 0, 0);                            \
      break;                                         \
    }                                                \
                                                     \
    execl(exe, exe, __VA_ARGS__, (char*)NULL);       \
    exit(errno);                                     \
  } while (0)

#define fork_exec_path(exe, ...)                 \
  do {                                           \
    printd("fork_exec_path(\"" exe "\")\n");     \
    const pid_t pid = fork();                    \
    if (pid == -1) {                             \
      print("fail exec_path\n");                 \
      break;                                     \
    } else if (pid > 0) {                        \
      printd("forked %d fork_exec_path\n", pid); \
      waitpid(pid, 0, 0);                        \
      break;                                     \
    }                                            \
                                                 \
    execlp(exe, exe, __VA_ARGS__, (char*)NULL);  \
    exit(errno);                                 \
  } while (0)

static FILE* kmsg_f = 0;

#define print(...)                  \
  do {                              \
    if (kmsg_f) {                   \
      fprintf(kmsg_f, __VA_ARGS__); \
      break;                        \
    }                               \
                                    \
    printf(__VA_ARGS__);            \
  } while (0)

#if 1
#define printd(...)     \
  do {                  \
    print(__VA_ARGS__); \
  } while (0)
#else
#define printd(...)
#endif

static inline void exec_absolute_path(const char* exe) {
  printd("exec_absolute_path(\"%s\")\n", exe);
  execl(exe, exe, (char*)NULL);
  exit(errno);
}

static inline FILE* log_open_kmsg(void) {
  kmsg_f = fopen("/dev/kmsg", "w");
  if (!kmsg_f) {
    print("open(\"/dev/kmsg\", \"w\"), %d = errno\n", errno);
    return kmsg_f;
  }

  setvbuf(kmsg_f, 0, _IOLBF, 0);
  return kmsg_f;
}

static inline long loop_ctl_get_free(void) {
  autoclose const int loopctlfd = open("/dev/loop-control", O_RDWR | O_CLOEXEC);
  if (loopctlfd < 0) {
    print("open(\"/dev/loop-control\", O_RDWR | O_CLOEXEC) = %d %d (%s)\n",
          loopctlfd, errno, strerror(errno));
    return 0;  // in error just try and continue with loop0
  }

  const long devnr = ioctl(loopctlfd, LOOP_CTL_GET_FREE);
  if (devnr < 0) {
    print("ioctl(%d, LOOP_CTL_GET_FREE) = %ld %d (%s)\n", loopctlfd, devnr,
          errno, strerror(errno));
    return 0;  // in error just try and continue with loop0
  }

  return devnr;
}

static inline int loop_configure(const long devnr,
                                 const int filefd,
                                 char** loopdev,
                                 const char* file) {
  const struct loop_config loopconfig = {
      .fd = (unsigned int)filefd,
      .block_size = 0,
      .info = {.lo_device = 0,
               .lo_inode = 0,
               .lo_rdevice = 0,
               .lo_offset = 0,
               .lo_sizelimit = 0,
               .lo_number = 0,
               .lo_encrypt_type = LO_CRYPT_NONE,
               .lo_encrypt_key_size = 0,
               .lo_flags = LO_FLAGS_PARTSCAN,
               .lo_file_name = "",
               .lo_crypt_name = "",
               .lo_encrypt_key = "",
               .lo_init = {0, 0}},
      .__reserved = {0, 0, 0, 0, 0, 0, 0, 0}};
  strncpy((char*)loopconfig.info.lo_file_name, file, LO_NAME_SIZE - 1);
  if (asprintf(loopdev, "/dev/loop%ld", devnr) < 0)
    return -1;

  autoclose const int loopfd = open(*loopdev, O_RDWR | O_CLOEXEC);
  if (loopfd < 0) {
    print("open(\"%s\", O_RDWR | O_CLOEXEC) = %d %d (%s)\n", *loopdev, loopfd,
          errno, strerror(errno));
    return errno;
  }

  if (ioctl(loopfd, LOOP_CONFIGURE, &loopconfig) < 0) {
    print("ioctl(%d, LOOP_CONFIGURE, %p) %d (%s)\n", loopfd, (void*)&loopconfig,
          errno, strerror(errno));
    return errno;
  }

  return 0;
}

static inline int losetup(char** loopdev, const char* file) {
  autoclose const int filefd = open(file, O_RDONLY | O_CLOEXEC);
  if (filefd < 0) {
    print("open(\"%s\", O_RDONLY| O_CLOEXEC) = %d %d (%s)\n", file, filefd,
          errno, strerror(errno));
    return errno;
  }

  const int ret = loop_configure(loop_ctl_get_free(), filefd, loopdev, file);
  if (ret)
    return ret;

  return 0;
}

/* remove all files/directories below dirName -- don't cross mountpoints */
static inline int recursiveRemove(int fd) {
  struct stat rb;
  DIR* dir;
  int rc = -1;
  int dfd;

  if (!(dir = fdopendir(fd))) {
    print("failed to open directory\n");
    goto done;
  }

  /* fdopendir() precludes us from continuing to use the input fd */
  dfd = dirfd(dir);
  if (fstat(dfd, &rb)) {
    print("stat failed\n");
    goto done;
  }

  while (1) {
    struct dirent* d;
    int isdir = 0;

    errno = 0;
    if (!(d = readdir(dir))) {
      if (errno) {
        print("failed to read directory\n");
        goto done;
      }

      break; /* end of directory */
    }

    if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, "..") ||
        !strcmp(d->d_name, "initoverlayfs"))
      continue;
#ifdef _DIRENT_HAVE_D_TYPE
    if (d->d_type == DT_DIR || d->d_type == DT_UNKNOWN)
#endif
    {
      struct stat sb;

      if (fstatat(dfd, d->d_name, &sb, AT_SYMLINK_NOFOLLOW)) {
        print("stat of %s failed\n", d->d_name);
        continue;
      }

      /* skip if device is not the same */
      if (sb.st_dev != rb.st_dev)
        continue;

      /* remove subdirectories */
      if (S_ISDIR(sb.st_mode)) {
        int cfd;

        cfd = openat(dfd, d->d_name, O_RDONLY);
        if (cfd >= 0)
          recursiveRemove(cfd); /* it closes cfd too */
        isdir = 1;
      }
    }

    if (unlinkat(dfd, d->d_name, isdir ? AT_REMOVEDIR : 0))
      print("failed to unlink %s", d->d_name);
  }

  rc = 0; /* success */
done:
  if (dir)
    closedir(dir);
  else
    close(fd);

  return rc;
}

static inline int move_chroot_chdir(const char* newroot) {
  printd("move_chroot_chdir(\"%s\")\n", newroot);
  if (mount(newroot, "/", NULL, MS_MOVE, NULL) < 0) {
    print("failed to mount moving %s to /\n", newroot);
    return -1;
  }

  if (chroot(".")) {
    print("failed to change root\n");
    return -1;
  }

  if (chdir("/")) {
    print("cannot change directory to %s\n", "/");
    return -1;
  }

  return 0;
}

static inline int switchroot_move(const char* newroot) {
  if (chdir(newroot)) {
    print("failed to change directory to %s", newroot);
    return -1;
  }

  autoclose const int cfd = open("/", O_RDONLY);
  if (cfd < 0) {
    print("cannot open %s", "/");
    return -1;
  }

  if (move_chroot_chdir(newroot))
    return -1;

  switch (fork()) {
    case 0: /* child */
    {
      struct statfs stfs;

      if (fstatfs(cfd, &stfs) == 0 &&
          (stfs.f_type == RAMFS_MAGIC || stfs.f_type == TMPFS_MAGIC))
        recursiveRemove(cfd);
      else {
        print("old root filesystem is not an initramfs");
        close(cfd);
      }

      exit(EXIT_SUCCESS);
    }
    case -1: /* error */
      break;

    default: /* parent */
      return 0;
  }

  return -1;
}

static inline int stat_oldroot_newroot(const char* newroot,
                                       struct stat* newroot_stat,
                                       struct stat* oldroot_stat) {
  if (stat("/", oldroot_stat) != 0) {
    print("stat of %s failed\n", "/");
    return -1;
  }

  if (stat(newroot, newroot_stat) != 0) {
    print("stat of %s failed\n", newroot);
    return -1;
  }

  return 0;
}

static inline int switchroot(const char* newroot) {
  /*  Don't try to unmount the old "/", there's no way to do it. */
  const char* umounts[] = {"/dev", "/proc", "/sys", "/run", NULL};
  struct stat newroot_stat, oldroot_stat, sb;
  if (stat_oldroot_newroot(newroot, &newroot_stat, &oldroot_stat))
    return -1;

  for (int i = 0; umounts[i] != NULL; ++i) {
    autofree char* newmount;
    if (asprintf(&newmount, "%s%s", newroot, umounts[i]) < 0) {
      print(
          "asprintf(%p, \"%%s%%s\", \"%s\", \"%s\") MS_NODEV, NULL) %d (%s)\n",
          (void*)newmount, newroot, umounts[i], errno, strerror(errno));
      return -1;
    }

    if ((stat(umounts[i], &sb) == 0) && sb.st_dev == oldroot_stat.st_dev) {
      /* mount point to move seems to be a normal directory or stat failed */
      continue;
    }

    if ((stat(newmount, &sb) != 0) || (sb.st_dev != newroot_stat.st_dev)) {
      /* mount point seems to be mounted already or stat failed */
      umount2(umounts[i], MNT_DETACH);
      continue;
    }

    if (mount(umounts[i], newmount, NULL, MS_MOVE, NULL) < 0) {
      print("failed to mount moving %s to %s", umounts[i], newmount);
      print("forcing unmount of %s", umounts[i]);
      umount2(umounts[i], MNT_FORCE);
    }
  }

  return switchroot_move(newroot);
}

static inline int mount_proc_sys_dev(void) {
  if (mount("proc", "/proc", "proc", MS_NOSUID | MS_NOEXEC | MS_NODEV, NULL)) {
    print(
        "mount(\"proc\", \"/proc\", \"proc\", MS_NOSUID | MS_NOEXEC | "
        "MS_NODEV, NULL) %d (%s)\n",
        errno, strerror(errno));
    return errno;
  }

  if (mount("sysfs", "/sys", "sysfs", MS_NOSUID | MS_NOEXEC | MS_NODEV, NULL)) {
    print(
        "mount(\"sysfs\", \"/sys\", \"sysfs\", MS_NOSUID | MS_NOEXEC | "
        "MS_NODEV, NULL) %d (%s)\n",
        errno, strerror(errno));
    return errno;
  }

  if (mount("devtmpfs", "/dev", "devtmpfs", MS_NOSUID | MS_STRICTATIME,
            "mode=0755,size=4m")) {
    print(
        "mount(\"devtmpfs\", \"/dev\", \"devtmpfs\", MS_NOSUID | "
        "MS_STRICTATIME, \"mode=0755,size=4m\") %d (%s)\n",
        errno, strerror(errno));
    return errno;
  }

  return 0;
}

static inline void udev_trigger(const char* udev_trigger) {
  if (!udev_trigger)
    fork_exec_path("udevadm", "trigger", "--type=devices", "--action=add",
                   "--subsystem-match=module", "--subsystem-match=block",
                   "--subsystem-match=virtio", "--subsystem-match=pci",
                   "--subsystem-match=nvme");
}

static inline int convert_bootfs(conf* c) {
  if (!c->bootfs.val || !c->bootfs.val[0])
    return -4;

  const char* token = strtok(c->bootfs.val, "=");
  autofree char* bootfs_tmp = 0;
  if (!strcmp(token, "LABEL")) {
    token = strtok(NULL, "=");
    if (asprintf(&bootfs_tmp, "/dev/disk/by-label/%s", token) < 0)
      return -1;

    swap(c->bootfs.scoped, bootfs_tmp);
    c->bootfs.val = c->bootfs.scoped;
    return 0;
  } else if (!strcmp(token, "UUID")) {
    token = strtok(NULL, "=");
    if (asprintf(&bootfs_tmp, "/dev/disk/by-uuid/%s", token) < 0)
      return -2;

    swap(c->bootfs.scoped, bootfs_tmp);
    c->bootfs.val = c->bootfs.scoped;
    return 0;
  }

  printd("convert_bootfs(%p)\n", (void*)c);

  return -3;
}

static inline void mounts(const conf* c) {
  if (!c->bootfs.val)
    return;

  if (mount(c->bootfs.val, "/boot", c->bootfstype.val, 0, NULL))
    print(
        "mount(\"%s\", \"/boot\", \"%s\", 0, NULL) "
        "%d (%s)\n",
        c->bootfs.val, c->bootfstype.val, errno, strerror(errno));

  autofree char* dev_loop = 0;
  if (c->fs.val && losetup(&dev_loop, c->fs.val))
    print("losetup(\"%s\", \"%s\") %d (%s)\n", dev_loop, c->fs.val, errno,
          strerror(errno));

  if (mount(dev_loop, "/initrofs", c->fstype.val, MS_RDONLY, NULL))
    print(
        "mount(\"%s\", \"/initrofs\", \"%s\", MS_RDONLY, NULL) "
        "%d (%s)\n",
        dev_loop, c->fstype.val, errno, strerror(errno));

  if (mount("overlay", "/initoverlayfs", "overlay", 0,
            "redirect_dir=on,lowerdir=/initrofs,upperdir=/overlay/"
            "upper,workdir=/overlay/work"))
    print(
        "mount(\"overlay\", \"/initoverlayfs\", \"overlay\", 0, "
        "\"redirect_dir=on,lowerdir=/initrofs,upperdir=/overlay/"
        "upper,workdir=/overlay/work\") %d (%s)\n",
        errno, strerror(errno));

  if (mount("/boot", "/initoverlayfs/boot", c->bootfstype.val, MS_MOVE, NULL))
    print(
        "mount(\"/boot\", \"/initoverlayfs/boot\", \"%s\", MS_MOVE, NULL) "
        "%d (%s)\n",
        c->bootfstype.val, errno, strerror(errno));
}

int main(void) {
  mount_proc_sys_dev();
  autofclose FILE* kmsg_f_scoped = log_open_kmsg();
  kmsg_f = kmsg_f_scoped;
  pid_t pid;
  fork_exec_absolute_no_wait(pid, "/lib/systemd/systemd-udevd", "--daemon");
  autofree_conf conf conf = {.bootfs = {0, 0},
                             .bootfstype = {0, 0},
                             .fs = {0, 0},
                             .fstype = {0, 0},
                             .udev_trigger = {0, 0}};
  read_conf("/etc/initoverlayfs.conf", &conf);
  waitpid(pid, 0, 0);
  udev_trigger(conf.udev_trigger.val);
  convert_bootfs(&conf);
  fork_exec_absolute_no_wait(pid, "/usr/sbin/modprobe", "loop");
  fork_exec_path("udevadm", "wait", conf.bootfs.val);
  waitpid(pid, 0, 0);
  errno = 0;

  mounts(&conf);
  if (switchroot("/initoverlayfs")) {
    print("switchroot(\"/initoverlayfs\") %d (%s)\n", errno, strerror(errno));
    return 0;
  }

  exec_absolute_path("/sbin/init");
  exec_absolute_path("/etc/init");
  exec_absolute_path("/bin/init");
  exec_absolute_path("/bin/sh");

  return 0;
}
