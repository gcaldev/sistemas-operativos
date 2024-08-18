#!/bin/bash

./fisopfs prueba/ -n /fisopfs/persist-test

file="prueba/test04_file_remove.txt"
touch $file

rm $file

if [ ! -e $file ]; then
    echo "TEST 04 [OK]: Archivo $file se elimina correctamente."
    retval=0
else
    echo "TEST 04 [FAIL]: Archivo $file se elimina correctamente."
    retval=1
fi

umount prueba
exit "$retval"