# zvenv
Virtualenv for everything

## Install
Distributed as a small static binary

## Usage
```
USAGE:
zvenv images                    list downloadable boxes
zvenv pull DISTRO:RELEASE       downloads a new box
zvenv run BOX *CMDS             run command in a box
zvenv cp SOURCE_BOX NEW_BOX     duplicate a box
zvenv ls                        list boxes
zvenv mv OLD_NAME NEW_NAME      rename a box
zvenv do *CMDS                  run in the box named "default"
```

## Example
```
$ zvenv pull ubuntu:focal
$ zvenv cp ubuntu:focal myproject
$ zvenv run myproject apt update
$ zvenv run myproject apt install python-pandas
$ ... manage other dependencies here
$ zvenv run myproject python3 myscript.py
```


## A note on arch linux
You need to remove the entry `CheckSpace` in `/etc/pacman.conf`

## FAQ

### Why?

Don't dump too much cruft in your root Linux installation, install your dependencies in zvenv instances instead.

### Does it run graphical applications?
Yes.

### Does sound work inside the virtualenvs?
No, not out of the box.

### Does it expose binded ports?
Yes, same network device.

### Can I still access all my files in /home
Yes.

