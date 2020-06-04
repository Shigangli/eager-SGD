#!/bin/bash


n=$1
for ((i=0; i<300; i++)); do 
    echo -n "Run $i "; 
    mpirun --oversubscribe -n $n sh -c './activation_tree_multiple > logs/out.$OMPI_COMM_WORLD_RANK'; 
    passc=$(eval cat logs/out.{0..$((n-1))} | grep PASSED | wc -l);
    if [[ "$passc" == "$n" ]]; then echo "OK"; 
    else 
        echo "FAIL"; 
        eval cat logs/out.{0..$((n-1))} | grep PASSED
        break; 
    fi; 

done;
