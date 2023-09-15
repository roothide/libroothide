#!/usr/bin/sh

/usr/bin/find / -xdev -type l | /usr/libexec/updatelink >/dev/null 2>&1

#jbfirmlinks
/usr/bin/find /private/var/ -xdev -type l | /usr/libexec/updatelink >/dev/null 2>&1
