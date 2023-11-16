#!/bin/bash

if [[ $# -ne 1 ]]
then
	echo "requires python script"
	exit 1
fi

make
stdbuf -i0 -o0 -e0 ./tiny | python3 $1
