#!/bin/bash

./fisopfs prueba/ -n /fisopfs/persist-test

file="prueba/test05_dir_remove.txt"
mkdir $file

rmdir $file

if [ ! -e $file ]; then
    echo "TEST 05 [OK]: Directorio $file se elimina exitosamente."
    retval=0
else
    echo "TEST 05 [FAIL]: Directorio $file se elimina exitosamente."
    retval=1
fi

umount prueba
exit "$retval"