#!/bin/bash

./fisopfs prueba/ -n /fisopfs/persist-test

file="prueba/append.txt"

initial_value="hola!"
appended_value="chau!"

echo "$initial_value" > $file
first_value_read=$(cat $file)

echo "$appended_value" >> $file
final_value_read=$(cat $file)


expected_value="$initial_value
$appended_value"


if [ "$initial_value" = "$first_value_read" ] && [ "$final_value_read" = "$expected_value" ]; then
    echo "TEST 13 [OK]: Append archivo $file se crea exitosamente."
    retval=0
else
    echo "TEST 13 [FAIL]: Append archivo $file error."
    retval=1
fi

umount prueba
exit "$retval"