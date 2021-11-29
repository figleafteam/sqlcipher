#!/bin/bash

DIR=sqlcipher-4.5.0-sqlite-3.36.0

./configure --enable-tempstore=yes --with-crypto-lib=none CFLAGS="-DSQLITE_HAS_CODEC -DSQLITE_TEMP_STORE=2"

mkdir $DIR
cp sqlite3.c $DIR

mkdir $DIR/include
cp sqlite3.h $DIR/include/

zip $DIR.zip $DIR -r

rm -rf $DIR