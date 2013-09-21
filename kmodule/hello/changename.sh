#!/bin/sh

if [ "x$1" == "x" ]; then
    echo "./changename.sh 'Module name'"
    echo "Module name not specified."
    exit 1
fi

sed -i "{s/hello/$1/g}" Makefile hello.c
mv hello.c $1.c
