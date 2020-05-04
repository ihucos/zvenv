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

char* zvenv_data = NULL;

#define QUOTE(...) #__VA_ARGS__
#define CMD(a) if (strcmp(argv[1], a) == 0)

const char *cmd_images = QUOTE(
curl --fail --silent --show-error --location "$LXC_INDEX_URL"
| awk -F";" '$3 == "'"$ARCH"'" && $4 == "default" { print $1":"$2 }'
);

const char *cmd_pull = QUOTE(
if [ -d "$ZVENV_DATA/$1" ]; then
  echo "zvenv: $1 already exists";
  exit 1;
fi;
match=$(curl --progress-bar --fail --location $LXC_INDEX_URL |
  awk -F";" '$3 == "'"$ARCH"'" && $4 == "default" { print $1":"$2" "$6 }' |
  awk -v a="$1" 'a == $1 { print $2 }'
);
if [ -z "$match" ]; then
  echo "zvenv error: \"$1\" not listed in $LXC_HOME_URL/" >&2;
  exit 1;
fi;
rootfs="${LXC_HOME_URL}${match}"rootfs.tar.xz;
mkdir "$ZVENV_DATA/$1";
curl --progress-bar --fail --location "$rootfs" | tar -C "$ZVENV_DATA/$1" -xJf -;
rm "$ZVENV_DATA/$1/etc/resolv.conf";
touch "$ZVENV_DATA/$1/etc/resolv.conf";
echo "Done";
);

int fatal(char *format, ...) {
  va_list args;
  va_start(args, format);
  va_end(args);
  fprintf(stderr, "zvenv: ");
  vfprintf(stderr, format, args);
  if (errno != 0)
    fprintf(stderr, ": %s", strerror(errno));
  fprintf(stderr, "\n");
  exit(1);
}


void usage(){
  fprintf(stderr, "USAGE:\n");
  fprintf(stderr, "zvenv images                    list downloadable virtualenvs\n");
  fprintf(stderr, "zvenv pull DISTRO:RELEASE       downloads a virtualenv\n");
  fprintf(stderr, "zvenv run VENV *CMDS            run command in virtualenv\n");
  fprintf(stderr, "zvenv cp SOURCE_VENV NEW_VENV   duplicates a virtualenv\n");
  fprintf(stderr, "zvenv ls                        list virtualenvs\n");
  fprintf(stderr, "zvenv mv OLD_NAME NEW_NAME      rename a virtualenv\n");
  fprintf(stderr, "zvenv rm VENV                   remove a virtualenv\n");
  fprintf(stderr, "zvenv do *CMDS                  run in the virtualenv named \"default\"\n");
  exit(1);
}

void run_virtualenv(const char *zvenv_data, const char* name, char** argv){
  pl_setup_mount_ns();

  char *rootfs = NULL;
  if (asprintf(&rootfs, "%s/%s", zvenv_data, name) == -1)
    fatal("asprintf");

  char *origpwd = get_current_dir_name();
  if (chdir(rootfs) == -1){
    if (errno == ENOENT){
      errno = 0;
      fatal("no such virtualenv: %s", name);
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
  pl_whitelist_envs_from_env("ZVENV_EXPORT");
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
      execlp("mkdir", "mkdir", "-p", zvenv_data, NULL);
      fatal("execlp");
    }
    int wstatus;
    wait(&wstatus);
}

char *init_zvenv_data_variable(){
  struct passwd *pw = getpwuid(getuid());
  const char *homedir = pw->pw_dir;
  if (asprintf(&zvenv_data, "%s/.local/lib/zvenv", homedir) == -1)
    fatal("asprintf");
  return zvenv_data;
}

void chdir_zvenv_data(){
  if (chdir(zvenv_data) == -1){
    fatal("chdir");
  }
}

int main(int argc, char* argv[]) {
  if (argc < 2){
      usage();
  }
  init_zvenv_data_variable();
  init_data_dir();

  char *zvenv_data_env = NULL;
  if (asprintf(&zvenv_data_env, "ZVENV_DATA=%s", zvenv_data) == -1)
    fatal("asprintf");

  putenv(zvenv_data_env);
  putenv("LXC_INDEX_URL=https://images.linuxcontainers.org/meta/1.0/index-user");
  putenv("LXC_HOME_URL=https://images.linuxcontainers.org");
  putenv("ARCH=amd64");

  if (getuid()) pl_setup_user_ns();

  CMD("run") {
    if (argc < 4) usage();
    run_virtualenv(zvenv_data, argv[2], argv + 3);
  }

  CMD("ls") {
    chdir_zvenv_data();
    execlp("ls", "ls", "-1", NULL);
    fatal("execlp");
  }

  CMD("mv") {
    if (argc < 4) usage();
    chdir_zvenv_data();
    execlp("mv", "mv", "--", argv[2], argv[3], NULL);
    fatal("execlp");
  }

  CMD("rm") {
    if (argc < 3) usage();
    chdir_zvenv_data();
    execlp("rm", "rm", "-r", "--", argv[2], NULL);
    fatal("execlp");
  }

  CMD("cp") {
    if (argc < 4) usage();
    chdir_zvenv_data();
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
    run_virtualenv(zvenv_data, "default", argv + 2);
  }

  usage();
}
