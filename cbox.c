

#define _GNU_SOURCE
#include "errno.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "plash.h"

#define PRESET_PATH "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"

#define QUOTE(...) #__VA_ARGS__

const char *cmd_list = QUOTE(
mkdir -p "$CBOX_DATA";
ls -1 "$CBOX_DATA";
);

const char *cmd_rename = QUOTE(
cd "$CBOX_DATA";
mv "$1" "$2";
);

const char *cmd_distros = QUOTE(
curl --fail --silent --show-error --location "$LXC_INDEX_URL" | awk -F";" '$3 == "'"$ARCH"'" && $4 == "default" { print $1":"$2 }' | xargs;
);

const char *cmd_fetch = QUOTE(
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


const char *usage = QUOTE(
usage is here\n
hyper
);

//const char *usage = QUOTE(
//usage:
//cbox list|ls
//cbox ls-distros
//cbox new alpine alpine:3.8
////cbox link alpine /usr/local/bin/gimp ~/.local/bin/alpine-gimp 
//cbox remove|rm|del|delete alpine
//cbox rename alpine myalpine
//cbox exec alpine bash
//)


void fatal(const char* err){
  fprintf(stderr, "cbox: %s\n", err);
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
      fatal("no such box");
    }
    pl_fatal("chdir");
  }
  
  pl_bind_mount("/dev",             "./dev");
  pl_bind_mount("/home",            "./home");
  pl_bind_mount("/proc",            "./proc");
  pl_bind_mount("/root",            "./root");
  pl_bind_mount("/sys",             "./sys");
  pl_bind_mount("/tmp",             "./tmp");
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
  
  /* exec away */
  execvp(argv[0], argv);
  pl_fatal("exec %s", argv[0]);

}

void cbox_usage(){
  fatal(usage);
}

void shell(const char* cmd, const char* arg1, const char* arg2){
    execlp("sh", "sh", "-uec", cmd, "--", arg1, arg2, NULL);
    pl_fatal("could not execlp");
}


int main(int argc, char* argv[]) {
  if (argc < 2){
      cbox_usage();
  }

  char* cbox_data = "/home/ihucos/.local/lib/cbox";
  putenv("CBOX_DATA=/home/ihucos/.local/lib/cbox"); // XXX hardcoded usersame!
  putenv("LXC_INDEX_URL=https://images.linuxcontainers.org/meta/1.0/index-user");
  putenv("LXC_HOME_URL=https://images.linuxcontainers.org");
  putenv("ARCH=amd64");

  if (getuid()) pl_setup_user_ns();

  if ((strcmp(argv[1], "ls") == 0) ||
     (strcmp(argv[1], "list") == 0))
  {
    shell(cmd_list, NULL, NULL);
  }

  if ((strcmp(argv[1], "rename") == 0) ||
     (strcmp(argv[1], "move") == 0) ||
     (strcmp(argv[1], "mv") == 0))
  {
    if (argc < 4){
        fatal("cbox mv: needs 2 args");
    }
    shell(cmd_rename, argv[2], argv[3]);
  }

  if ((strcmp(argv[1], "ls-distros") == 0) ||
     (strcmp(argv[1], "list-distros") == 0) ||
     (strcmp(argv[1], "distros") == 0))
  {
    shell(cmd_distros, argv[2], argv[3]);
  }

  if ((strcmp(argv[1], "fetch") == 0) ||
     (strcmp(argv[1], "download") == 0) ||
     (strcmp(argv[1], "pull") == 0))
  {
    if (argc < 3){
        fatal("cbox mv: needs 1 args");
    }
    shell(cmd_fetch, argv[2], NULL);
  }

  if ((strcmp(argv[1], "run") == 0) ||
     (strcmp(argv[1], "execute") == 0) ||
     (strcmp(argv[1], "exec") == 0))
  {
    if (argc < 4){
        fatal("cbox run: needs more args");
    }
    run(cbox_data, argv[2], argv + 3);
  }
}
