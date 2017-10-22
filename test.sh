#!/bin/bash
set -e

# test input for debugging the server. This will be removed when automatic testing is done.
#(sleep 1 && echo -n "{test}{hello}" && sleep 1 && echo -n "{world}") | nc -q 1 127.0.0.1 2000
(sleep 1 && \
	echo -n "{\"version\":1,\"data\":{\"message\":\"blah blah software broke\"},\"type\":\"SOFT_ERROR\"}" && \
	sleep 1 && \
	echo -n "{\"version\":1,\"data\":{\"valve_no\":1,\"test_point_no\":2,\"test_point_high\":true},\"type\":\"HARD_ERROR\"}")\
    | nc -q 1 127.0.0.1 2000
