#!/bin/bash

export LD_LIBRARY_PATH="./.libs:$LD_LIBRARY_PATH"

valgrind --leak-check=full --show-leak-kinds=all .libs/$1

