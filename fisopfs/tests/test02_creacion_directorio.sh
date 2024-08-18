#!/bin/bash

./fisopfs prueba/ -n /fisopfs/persist-test

file="prueba/test02_dir.txt"

mkdir $file

if [ -e $file ]; then
    echo "TEST 02 [OK]: Directorio $file se crea exitosamente."
    retval=0
else
    echo "TEST 02 [FAIL]: Directorio $file se crea exitosamente."
    retval=1
fi

umount prueba
exit "$retval"