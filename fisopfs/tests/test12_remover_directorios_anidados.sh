#!/bin/bash

./fisopfs prueba/ -n /fisopfs/persist-test

dir="prueba/base_dir" 
subdir="$dir/subdir"

mkdir $dir
mkdir $subdir

result=$(rmdir $dir 2>&1)
expected="rmdir: failed to remove '$dir': Directory not empty"

if [ "$result" = "$expected" ]; then
    echo "TEST 12 [OK]: Directorio con subdirectorios no es removido."
    retval=0
else
    echo "TEST 12 [FAIL]: Directorio con subdirectorios fue removido inesperadamente."
    retval=1
fi

umount prueba
exit "$retval"