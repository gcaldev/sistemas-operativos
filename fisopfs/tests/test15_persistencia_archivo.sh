#!/bin/bash

./fisopfs prueba/ -n /fisopfs/persist-test

file="prueba/test_content.txt"
expected_content="ContenidoDeArchivo"


echo "$expected_content" > "$file"

# Desmontar y volver a montar FS
umount prueba
./fisopfs prueba/ -n /fisopfs/persist-test


result=$(cat $file)

if [ "$result" = "$expected_content" ]; then
    echo "TEST 15 [OK]: El archivo se creó y persistio correctamente: $file"
    retval=0
else
    echo "TEST 15 [FAIL]: Hubo un error al crear y persistir el archivo: $file"
    retval=1
fi

umount prueba
exit "$retval"
