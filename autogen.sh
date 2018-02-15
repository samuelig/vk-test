#!/bin/sh
aclocal -I m4 && \
rm -rf autom4te.cache/ && \
autoheader && \
automake --foreign &&
autoconf
./configure $@
