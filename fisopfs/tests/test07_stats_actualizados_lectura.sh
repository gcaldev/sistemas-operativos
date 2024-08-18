#!/bin/bash

./fisopfs prueba/
file="prueba/stats_read.txt"
echo "contenido" > $file

current_date=$(date +"%Y-%m-%d %H:%M:%S")
change_date="Change: $current_date"
modify_date="Modify: $current_date"
gid=$(id -g)
uid=$(id -u)



sleep 5
cat $file
after_5_seconds=$(date +"%Y-%m-%d %H:%M:%S")
access_date="Access: $after_5_seconds"
result=$(stat $file)



echo "$access_date access_date"
echo "$change_date change_date"
echo "$modify_date modify_date"

if [ "$(echo "$result" | grep "$uid")" ] && [ "$(echo "$result" | grep "$gid")" ] && [ "$(echo "$result" | grep "$access_date")" ] && [ "$(echo "$result" | grep "$change_date")" ] && [ "$(echo "$result" | grep "$modify_date")" ];
then
    echo "TEST 07 [OK]: Los stats al leer el archivo $file son correctos"
    retval=0
else
    echo "TEST 07 [FAIL]: Los stats al leer el archivo $file son correctos
    Esperado:
        - Contiene el gid: $gid
        - Contiene el uid: $uid
        - $access_date 
        - $change_date
        - $modify_date
    Resultado:
    $result"
    retval=1
fi

umount prueba
#chmod +x tests/stats_iniciales.sh
exit "$retval"