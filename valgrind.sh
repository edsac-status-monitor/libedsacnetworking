#!/bin/bash

export LD_LIBRARY_PATH="/home/lain/proj/libnetworking/.libs:$LD_LIBRARY_PATH"

valgrind --leak-check=full --show-leak-kinds=all .libs/$1

