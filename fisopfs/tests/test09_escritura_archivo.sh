#!/bin/bash

./fisopfs prueba/ -n /fisopfs/persist-test

file="prueba/test09_write_content.txt"
expected_content="Contenido de prueba"
touch $file


echo $expected_content > $file


result=$(cat $file)

if [ "$result" = "$expected_content"  ]; then
    echo "TEST 09 [OK]: Escritura de contenido a $file se realiza correctamente."
    retval=0
else
    echo "TEST 09 [FAIL]: Escritura de contenido a $file se realiza correctamente.
    Contenido esperado:
    $expected_content
    Resultado:
    $result"
    retval=1
fi

umount prueba
exit "$retval"