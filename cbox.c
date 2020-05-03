

#define _GNU_SOURCE
#include "errno.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "plash.h"

#define PRESET_PATH "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"

#define QUOTE(...) #__VA_ARGS__

const char *cmd_images = QUOTE(
curl --fail --silent --show-error --location "$LXC_INDEX_URL" | awk -F";" '$3 == "'"$ARCH"'" && $4 == "default" { print $1":"$2 }' | xargs;
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
curl --progress-bar --fail --location "$rootfs" | tar -C "$CBOX_DATA" --one-top-level="$1" -xJf -;
);


void fatal(const char* err){
  fprintf(stderr, "cbox: %s\n", err);
  exit(1);
}



void usage(){
  fprintf(stderr, "USAGE:\n");
  fprintf(stderr, "cbox pull DISTRO               downloads a new box\n");
  fprintf(stderr, "cbox exec BOX *CMDS            run command in box\n");
  fprintf(stderr, "cbox cp SOURCE_BOX NEW_BOX     copy a box\n");
  fprintf(stderr, "cbox images                    list downloadable boxes\n");
  fprintf(stderr, "cbox ls                        list boxes\n");
  fprintf(stderr, "cbox mv OLD_NAME NEW_NAME      rename a box\n");
  exit(1);
}

void run(char *cbox_data, char* name, char** argv){
  pl_setup_mount_ns();

  char *rootfs = NULL;
  if (asprintf(&rootfs, "%s/%s", cbox_data, name) == -1)
    pl_fatal("asprintf");

  char *origpwd = get_current_dir_name();
  if (chdir(rootfs) == -1){
    if (errno == ENOENT){
      fprintf(stderr, "no such box: %s\n", name);
      exit(1);
    }
    pl_fatal("chdir");
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
  
  chroot(".") != -1 || pl_fatal("chroot");
  pl_chdir(origpwd);
  
  execvp(argv[0], argv);
  pl_fatal("exec %s", argv[0]);

}

void shell(const char* cmd, const char* arg1, const char* arg2){
    execlp("sh", "sh", "-uec", cmd, "--", arg1, arg2, NULL);
    pl_fatal("could not execlp");
}

void init(const char* cbox_data){
    execlp("mkdir", "mkdir", "-p", cbox_data, NULL);
    pl_fatal("could not execlp");
}


int main(int argc, char* argv[]) {
  if (argc < 2){
      //cbox_usage();
  }

  char* cbox_data = "/home/ihucos/.local/lib/cbox";
  putenv("CBOX_DATA=/home/ihucos/.local/lib/cbox"); // XXX hardcoded usersame!
  putenv("LXC_INDEX_URL=https://images.linuxcontainers.org/meta/1.0/index-user");
  putenv("LXC_HOME_URL=https://images.linuxcontainers.org");
  putenv("ARCH=amd64");

  if (getuid()) pl_setup_user_ns();

  if (strcmp(argv[1], "exec") == 0) {
    if (argc < 4) usage();
    run(cbox_data, argv[2], argv + 3);
  }

  if (chdir(cbox_data) == -1){
    pl_fatal("chdir");
  }

  if (strcmp(argv[1], "ls") == 0) {
    execlp("ls", "ls", "-1", NULL);
    pl_fatal("execlp");
  }

  else if (strcmp(argv[1], "mv") == 0) {
    if (argc < 4) usage();
    execlp("mv", "mv", "--", argv[2], argv[3], NULL);
    pl_fatal("execlp");
  }

  else if (strcmp(argv[1], "cp") == 0) {
    if (argc < 4) usage();
    execlp("cp", "cp", "--", "--reflink=auto", "-r", argv[2], argv[3], NULL);
    pl_fatal("execlp");
  }

  else if (strcmp(argv[1], "images") == 0) {
    shell(cmd_images, argv[2], argv[3]);
  }

  else if (strcmp(argv[1], "pull") == 0) {
    if (argc < 3) usage();
    shell(cmd_pull, argv[2], NULL);
  }

  else if (strcmp(argv[1], "do") == 0) {
    run(cbox_data, "default", argv + 2);
  }

  usage();

}
