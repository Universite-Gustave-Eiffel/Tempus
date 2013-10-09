#!/bin/bash

rm log.*

for c in  1 2 7 8 9 16 32 64;
do
    for i in $(seq $c);
    do
        { ./src/tests/test_wps.py >& log.$c.$i & }
    done
    for job in `jobs -p`
    do
        wait $job 
    done
    echo done with $c clients
done

grep Ran log.* | sed 's/log\.//;s/\..*in//;s/s//'
