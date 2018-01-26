#!/bin/bash
cd build/
make

mkdir ../server ../client

mv server ../server/
mv client ../client/

cp examples/test.jpg examples/luigi.gif ../server/

make cleanall
cd ..
