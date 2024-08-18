#!/bin/bash

./fisopfs prueba/ -n /fisopfs/persist-test

parent_dir="prueba/directorio"
sub_dir="$parent_dir/subdirectorio"

mkdir $parent_dir
mkdir $sub_dir

# Desmontar y volver a montar FS
umount prueba
./fisopfs prueba/ -n /fisopfs/persist-test


if [ -e $sub_dir ]; then
    echo "TEST 16 [OK]: Directorio anidado $sub_dir se persiste exitosamente."
    retval=0
else
    echo "TEST 16 [FAIL]: Directorio anidado $sub_dir no se persiste exitosamente."
    retval=1
fi

umount prueba
exit "$retval"
