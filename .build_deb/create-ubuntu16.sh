#!/bin/bash

VERSION=`cat version`

rm .build_deb/ubuntu16/var/log/iqlogger/.gitkeep
rm .build_deb/ubuntu16/var/lib/iqlogger/.gitkeep
rm .build_deb/ubuntu16/usr/local/bin/.gitkeep
cd .build_deb/ubuntu16
fpm -s dir -t deb -a amd64 -n iqlogger -v ${VERSION}-xenial -m neogenie@yandex.ru \
    -d libtbb2 -d zlib1g \
    --pre-install ../pre-install.sh \
    --after-install ../post-install.sh \
    --vendor neogenie@yandex.ru \
    --config-files etc/iqlogger \
    etc/ usr/ var/
