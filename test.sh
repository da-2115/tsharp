#!/bin/bash

for i in {0..19}
do
    echo "Test ${i}"
    time ./tsharp examples/conway3.tsharp > out.txt
done