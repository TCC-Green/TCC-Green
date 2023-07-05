#!/bin/bash

THREADS=1
ALGORITHM="m"
OPTIMIZATION="O3"
NSIZE=40000
ITERATIONS=31

function compile {
    g++ /home/paulo/Desktop/TCC-Green/*.cpp -o /home/paulo/Desktop/TCC-Green/tccgreen -$OPTIMIZATION -fopenmp -Wall
}

function execute {
    sudo OMP_NUM_THREADS=$THREADS /home/paulo/Desktop/TCC-Green/tccgreen -o $OPTIMIZATION-$ALGORITHM$THREADS.txt -n $NSIZE -i $ITERATIONS -c $THREADS -$ALGORITHM -$ALGORITHM -$ALGORITHM -$ALGORITHM -$ALGORITHM
}

function testThreads {
    THREADS=12
    execute
    THREADS=10
    execute
    THREADS=8
    execute
    THREADS=6
    execute
    THREADS=4
    execute
    THREADS=2
    execute
    THREADS=1
    execute
}

function test {
    NSIZE=40000
    ITERATIONS=31
    ALGORITHM="m"
    testThreads

    NSIZE=40000
    ITERATIONS=31
    ALGORITHM="r"
    testThreads

    ALGORITHM="s"
    NSIZE=28000
    ITERATIONS=63
    testThreads
}

OPTIMIZATION="O3"
compile
test
OPTIMIZATION="O2"
compile
test
OPTIMIZATION="O1"
compile
test
OPTIMIZATION="O0"
compile
test