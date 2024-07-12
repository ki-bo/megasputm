#!/bin/bash

for i in $(seq -w 0 58); do
    upper_file=$(printf "%02d.LFL" $i)
    lower_file=$(printf "%02d.lfl" $i)
    c1541 -attach zak.d81 -write $upper_file $lower_file
done
