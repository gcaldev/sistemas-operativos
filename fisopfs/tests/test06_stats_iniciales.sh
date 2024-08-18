#!/bin/bash

./fisopfs prueba/ -n /fisopfs/persist-test
file="prueba/test06_initial_stats.txt"
touch $file

current_date=$(date +"%Y-%m-%d %H:%M:%S")
access_date="Access: $current_date"
change_date="Change: $current_date"
modify_date="Modify: $current_date"
gid=$(id -g)
uid=$(id -u)


result=$(stat $file)

echo "$access_date access_date"
echo "$change_date change_date"
echo "$modify_date modify_date"

if [ "$(echo "$result" | grep "$uid")" ] && [ "$(echo "$result" | grep "$gid")" ] && [ "$(echo "$result" | grep "$access_date")" ] && [ "$(echo "$result" | grep "$change_date")" ] && [ "$(echo "$result" | grep "$modify_date")" ];
then
    echo "TEST 06 [OK]: Los stats al crearse el archivo $file son correctos"
    retval=0
else
    echo "TEST 06 [FAIL]: Los stats al crearse el archivo $file son correctos
    Esperado:
        - Contiene el gid: $gid
        - Contiene el uid: $uid
        - $access_date 
        - $change_date
        - $modify_date
    Resultado:
    $result
    "
    retval=1
fi

umount prueba
exit "$retval"