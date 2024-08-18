#!/bin/bash

# Directorio donde estan todos los tests.sh
test_dir="./tests"

# Directorio temporal (se crea y borra para cada test)
scratchpad_dir="./prueba"

# Recompilar todo
make clean
make

# Scripts de prueba
sh_files=$(find $test_dir -maxdepth 1 -type f -name "*.sh")

retval=0;

for script in $sh_files; do
  echo "------------------------------------"
  echo "Ejecutando $script"

  # Preparar entorno para tests
  # Remover archivos de persistencia en este nivel
  persistent_files=$(find ./ -maxdepth 1 -type f -name "*.fisopfs")
  if [ -n "$persistent_files" ]; then
    rm $persistent_files
  fi
  
  # Borrar si existe y crear carpeta vacia
  if [ -e "$scratchpad_dir" ]; then
    umount -q "$scratchpad_dir" # Intentar desmontarlo si existe
    rm -r "$scratchpad_dir"
  fi
  mkdir $scratchpad_dir
  
  chmod +x "$script"
  
  # Ejecutar script
  ./"$script"
  script_return=$?

  if [ "$script_return" = 0 ]; then
     echo -e "\e[32mPrueba OK (return code 0)\e[0m"
  else
    echo -e "\e[31mPrueba Error (return code $script_return)\e[0m"
    retval=1
  fi
done


if [ "$retval" = 0 ]; then
   echo -e "\e[32m-------------------------------\e[0m"
   echo -e "\e[32m > Todos las pruebas OK\e[0m"
   echo -e "\e[32m-------------------------------\e[0m"
else
  echo -e "\e[31m--------------------------------\e[0m"
  echo -e "\e[31m > Algunas pruebas fallaron\e[0m"
  echo -e "\e[31m--------------------------------\e[0m"
fi

exit "$retval"

