#!/bin/bash

./fisopfs prueba/ -n /fisopfs/persist-test

file="prueba/test10_append_content.txt"
word1="Contenido original"
expected_content="Otro"


echo $word1 > $file
echo $expected_content > $file


result=$(cat $file)

if [ "$result" = "$expected_content"  ]; then
    echo "TEST 11 [OK]: Al escribir un contenido mas corto en $file se trunca el tamaño."
    retval=0
else
    echo "TEST 11 [FAIL]: Al escribir un contenido mas corto en $file se trunca el tamaño.
    Contenido esperado:
    $expected_content
    Resultado:
    $result"
    retval=1
fi

umount prueba
exit "$retval"