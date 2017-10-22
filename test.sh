#!/bin/bash
set -e

# test input for debugging the server. This will be removed when automatic testing is done.
(sleep 1 && echo -n {test}{hello} && sleep 1 && echo -n "{world}") | nc -q 1 127.0.0.1 2000
