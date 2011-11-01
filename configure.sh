#!/bin/sh
./configure --enable-maintainer-mode --prefix=/tmp/openocd/target --enable-ft2232-libftdi CFLAGS="-g -L/usr/local/lib -I/usr/local/include"

