#!/bin/bash

./fisopfs prueba/ -n /fisopfs/persist-test

parent_dir="prueba/directorio"
sub_dir="$parent_dir/subdirectorio"

mkdir $parent_dir
mkdir $sub_dir

if [ -e $sub_dir ]; then
    echo "TEST 14 [OK]: Directorio anidado $sub_dir se crea exitosamente."
    retval=0
else
    echo "TEST 14 [FAIL]: Directorio anidado $sub_dir se crea exitosamente."
    retval=1
fi

umount prueba
exit "$retval"