#!/bin/bash

./fisopfs prueba/ -n /fisopfs/persist-test

file="prueba/test_03_content.txt"
expected_content="Esto es una prueba con contenido"


echo "$expected_content" > "$file"

result=$(cat $file)


if [ "$result" = "$expected_content" ]; then
    echo "TEST 03 [OK]: El archivo $file se creó con contenido"
    retval=0
else
    echo "TEST 03 [FAIL]: El archivo $file se creó con contenido
    Contenido esperado: $expected_content
    Resultado: $result"
    retval=1
fi

umount prueba
exit "$retval"