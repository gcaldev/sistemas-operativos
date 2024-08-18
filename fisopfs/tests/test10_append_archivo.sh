#!/bin/bash

./fisopfs prueba/ -n /fisopfs/persist-test

file="prueba/test10_append_content.txt"
word1="Contenido original"
word2="Contenido appendeado"
expected_content="$word1
$word2"
echo $word1 > $file


echo $word2 >> $file


result=$(cat $file)

if [ "$result" = "$expected_content"  ]; then
    echo "TEST 10 [OK]: Append de contenido a $file se realiza correctamente."
    retval=0
else
    echo "TEST 10 [FAIL]: Append de contenido a $file se realiza correctamente.
    Contenido esperado:
    $expected_content
    Resultado:
    $result"
    retval=1
fi

umount prueba
exit "$retval"