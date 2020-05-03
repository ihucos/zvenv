#define _GNU_SOURCE
#include "errno.h"
#include "string.h"
#include "pwd.h"
#include "stdio.h"
#include "stdarg.h"
#include "stdlib.h"
#include "unistd.h"
#include "sys/wait.h"
#include "sys/types.h"

#include "plash.h"

#define PRESET_PATH "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"

char* cbox_data = NULL;

#define QUOTE(...) #__VA_ARGS__
#define CMD(a) if (strcmp(argv[1], a) == 0)

const char *cmd_images = QUOTE(
curl --fail --silent --show-error --location "$LXC_INDEX_URL"
| awk -F";" '$3 == "'"$ARCH"'" && $4 == "default" { print $1":"$2 }'
);

const char *cmd_pull = QUOTE(
if [ -d "$CBOX_DATA/$1" ]; then
  echo "cbox: $1 already exists";
  exit 1;
fi;
match=$(curl --progress-bar --fail --location $LXC_INDEX_URL |
  awk -F";" '$3 == "'"$ARCH"'" && $4 == "default" { print $1":"$2" "$6 }' |
  awk -v a="$1" 'a == $1 { print $2 }'
);
if [ -z "$match" ]; then
  echo "cbox error: \"$1\" not listed in $LXC_HOME_URL/" >&2;
  exit 1;
fi;
rootfs="${LXC_HOME_URL}${match}"rootfs.tar.xz;
mkdir "$CBOX_DATA/$1";
curl --progress-bar --fail --location "$rootfs" | tar -C "$CBOX_DATA/$1" -xJf -;
rm "$CBOX_DATA/$1/etc/resolv.conf";
touch "$CBOX_DATA/$1/etc/resolv.conf";
echo "Done";
);

int fatal(char *format, ...) {
  va_list args;
  va_start(args, format);
  va_end(args);
  fprintf(stderr, "cbox: ");
  vfprintf(stderr, format, args);
  if (errno != 0)
    fprintf(stderr, ": %s", strerror(errno));
  fprintf(stderr, "\n");
  exit(1);
}


void usage(){
  fprintf(stderr, "USAGE:\n");
  fprintf(stderr, "cbox images                    list downloadable boxes\n");
  fprintf(stderr, "cbox pull DISTRO:RELEASE       downloads a new box\n");
  fprintf(stderr, "cbox run BOX *CMDS             run command in a box\n");
  fprintf(stderr, "cbox cp SOURCE_BOX NEW_BOX     duplicate a box\n");
  fprintf(stderr, "cbox ls                        list boxes\n");
  fprintf(stderr, "cbox mv OLD_NAME NEW_NAME      rename a box\n");
  fprintf(stderr, "cbox do *CMDS                  run in the box named \"default\"\n");
  exit(1);
}

void run_box(const char *cbox_data, const char* name, char** argv){
  pl_setup_mount_ns();

  char *rootfs = NULL;
  if (asprintf(&rootfs, "%s/%s", cbox_data, name) == -1)
    fatal("asprintf");

  char *origpwd = get_current_dir_name();
  if (chdir(rootfs) == -1){
    if (errno == ENOENT){
      errno = 0;
      fatal("no such box: %s", name);
    }
    fatal("chdir");
  }
  
  pl_bind_mount("/dev", "./dev");
  pl_bind_mount("/home", "./home");
  pl_bind_mount("/proc", "./proc");
  pl_bind_mount("/root", "./root");
  pl_bind_mount("/sys", "./sys");
  pl_bind_mount("/tmp", "./tmp");
  pl_bind_mount("/etc/resolv.conf", "./etc/resolv.conf");
  
  putenv("PATH=" PRESET_PATH);
  pl_whitelist_env("TERM");
  pl_whitelist_env("DISPLAY");
  pl_whitelist_env("HOME");
  pl_whitelist_env("PATH");
  pl_whitelist_envs_from_env("PLASH_EXPORT");
  pl_whitelist_envs_from_env("CBOX_EXPORT");
  pl_whitelist_env(NULL);
  
  chroot(".") != -1 || fatal("chroot");
  pl_chdir(origpwd);

  pl_exec_add("/bin/sh");
  pl_exec_add("-lc");
  pl_exec_add("exec env \"$@\"");
  pl_exec_add("--");
  while (argv[0])
    pl_exec_add((argv++)[0]);
  pl_exec_add(NULL); // this calls exec
  fatal("exec %s", argv[0]);
}

void shell(const char* cmd, const char* arg1, const char* arg2){
    execlp("sh", "sh", "-uec", cmd, "--", arg1, arg2, NULL);
    fatal("execlp");
}

void init_data_dir(){
    if (!fork()){
      execlp("mkdir", "mkdir", "-p", cbox_data, NULL);
      fatal("execlp");
    }
    int wstatus;
    wait(&wstatus);
}

char *init_cbox_data_variable(){
  struct passwd *pw = getpwuid(getuid());
  const char *homedir = pw->pw_dir;
  if (asprintf(&cbox_data, "%s/.local/lib/cbox", homedir) == -1)
    fatal("asprintf");
  return cbox_data;
}

void chdir_cbox_data(){
  if (chdir(cbox_data) == -1){
    fatal("chdir");
  }
}

int main(int argc, char* argv[]) {
  if (argc < 2){
      usage();
  }
  init_cbox_data_variable();
  init_data_dir();

  char *cbox_data_env = NULL;
  if (asprintf(&cbox_data_env, "CBOX_DATA=%s", cbox_data) == -1)
    fatal("asprintf");

  putenv(cbox_data_env);
  putenv("LXC_INDEX_URL=https://images.linuxcontainers.org/meta/1.0/index-user");
  putenv("LXC_HOME_URL=https://images.linuxcontainers.org");
  putenv("ARCH=amd64");

  if (getuid()) pl_setup_user_ns();

  if (strcmp(argv[1], "run") == 0) {
    if (argc < 4) usage();
    run_box(cbox_data, argv[2], argv + 3);
  }

  CMD("ls") {
    chdir_cbox_data();
    execlp("ls", "ls", "-1", NULL);
    fatal("execlp");
  }

  CMD("mv") {
    if (argc < 4) usage();
    chdir_cbox_data();
    execlp("mv", "mv", "--", argv[2], argv[3], NULL);
    fatal("execlp");
  }

  CMD("rm") {
    if (argc < 3) usage();
    chdir_cbox_data();
    execlp("rm", "rm", "-r", "--", argv[2], NULL);
    fatal("execlp");
  }

  CMD("cp") {
    if (argc < 4) usage();
    chdir_cbox_data();
    execlp("cp", "cp", "--reflink=auto", "-r", "--", argv[2], argv[3], NULL);
    fatal("execlp");
  }

  CMD("images") {
    shell(cmd_images, argv[2], argv[3]);
  }

  CMD("pull") {
    if (argc < 3 || (strchr(argv[2], ':') == NULL)) usage();
    shell(cmd_pull, argv[2], NULL);
  }

  CMD("do") {
    run_box(cbox_data, "default", argv + 2);
  }

  usage();
}
