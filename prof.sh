#!/bin/bash
make profile
file="profile`date +%Y%m%d_%H%M%S`.txt"
./demo | tee "$file"
(gprof --brief ./demo) >> "$file"
gprof --brief --flat-profile ./demo
