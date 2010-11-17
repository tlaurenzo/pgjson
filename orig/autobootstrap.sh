#!/bin/bash

cd $(dirname $0)

mkdir -p m4
echo "Running aclocal"
if ! aclocal -I m4; then
   echo "Error running aclocal"
   exit 2
fi

echo "Running libtoolize"
if ! libtoolize; then
   echo "Error running libtoolize"
   exit 2
fi

echo "Running autoconf"
if ! autoconf; then
   echo "Error running autoconf"
   exit 2
fi

echo "Running autoheader"
if ! autoheader; then
   echo "Error running autoheader"
   exit 2
fi

echo "Running automake"
if ! automake --add-missing --foreign; then
   echo "Error running automake"
   exit 2
fi

echo "Configuring..."
export CFLAGS="-ggdb"
./configure


