#!/bin/bash

./fisopfs prueba/ -n /fisopfs/persist-test

file="prueba/test01_empty_file.txt"

touch $file

if [ -e $file ]; then
    echo "TEST 01 [OK]: Archivo $file se crea exitosamente."
    retval=0
else
    echo "TEST 01 [FAIL]: Archivo $file se crea exitosamente."
    retval=1
fi

umount prueba
exit "$retval"