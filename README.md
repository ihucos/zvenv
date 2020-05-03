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

```


## A note on arch linux
You need to remove the entry `CheckSpace` in `/etc/pacman.conf`

## Why?

Don't dump too much cruft in your root Linux installation, install your dependencies in cbox instances instead.
