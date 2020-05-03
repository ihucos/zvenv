# cbox
Virtualenv for everything

## Install
Distributed as a small static binary

## Usage
```
USAGE:
cbox images                    list downloadable boxes
cbox pull DISTRO:RELEASE       downloads a new box
cbox run BOX *CMDS             run command in a box
cbox cp SOURCE_BOX NEW_BOX     duplicate a box
cbox ls                        list boxes
cbox mv OLD_NAME NEW_NAME      rename a box
cbox do *CMDS                  run in the box named "default"
```

## Example
```
$ cbox pull ubuntu:focal
$ cbox cp ubuntu:focal myproject
$ cbox run myproject apt update
$ cbox run myproject apt install python-pandas
$ ... manage other dependencies here
$ cbox run myproject python3 myscript.py
```


## A note on arch linux
You need to remove the entry `CheckSpace` in `/etc/pacman.conf`

## FAQ

### Why?

Don't dump too much cruft in your root Linux installation, install your dependencies in cbox instances instead.

### Does it run graphical applicaitons?
Yes.

### Does sound work in other environments?
No, not out of the box.

### Does it expose binded ports?
Yes, same network device.


