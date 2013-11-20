#!/bin/bash

rm log.*

for c in 1200 #32 64 128 $(seq 16) 
do
    echo lauching $c clients 1>&2
    for i in $(seq $c);
    do
        { ./src/tests/test_wps.py >& log.$c.$i & }
    done
    for job in `jobs -p`
    do
        wait $job 
    done
    echo done with $c clients 1>&2
done

grep Ran log.* | sed 's/log\.//;s/\..*in//;s/s//'
