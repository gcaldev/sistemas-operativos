# shell

### Búsqueda en $PATH

#### ¿Cuáles son las diferencias entre la syscall execve(2) y la familia de wrappers proporcionados por la librería estándar de C (libc) exec(3)?

Nivel de Abstracción: execve(2) es una llamada al sistema (syscall), mientras que exec(3) son wrappers de alto nivel proporcionados por la librería estándar de C (libc).

Argumentos: execve(2) requiere argumentos más detallados, como la ruta del programa, un array de argumentos y un array de variables de entorno. exec(3) ofrece variantes que simplifican la especificación de estos elementos.

Variables de Entorno: Con execve(2) debes gestionar las variables de entorno manualmente. exec(3) generalmente utiliza el entorno del proceso actual.

Facilidad de Uso: exec(3) es más fácil de usar y más flexible gracias a sus diferentes variantes (execl, execp, execle, etc.), que permiten diferentes formas de especificar argumentos y el entorno.

Portabilidad: Las funciones de exec(3) son más portables entre diferentes sistemas UNIX, ya que son parte de la libc.

#### ¿Puede la llamada a exec(3) fallar? ¿Cómo se comporta la implementación de la shell en ese caso?

Sí, la llamada a exec(3) puede fallar por diversas razones, como falta de permisos, archivo inexistente o falta de recursos del sistema. En el caso de que exec falle, la implementación típica de una shell suele retornar el control al proceso padre (la propia shell), y generalmente se muestra un mensaje de error.

En el contexto de una shell, si exec falla, el proceso que intentó reemplazar su imagen sigue en ejecución y puede manejar el error. Por lo general, se devuelve un código de error específico para indicar el tipo de fallo.

---

### Procesos en segundo plano

#### Detallar cuál es el mecanismo utilizado para implementar procesos en segundo plano.

(parte 1) G:
En este caso, el proceso principal utiliza la función waitpid() para monitorear el estado de los procesos en segundo plano.

El flag WNOHANG en waitpid() permite que el proceso principal continúe su ejecución sin quedar bloqueado esperando que los procesos hijos (en segundo plano) terminen. Es decir, el proceso principal puede hacer otras cosas y no se detiene para esperar a los procesos hijos.

El segundo argumento de waitpid() se utiliza para almacenar el estado que devuelve el proceso hijo cuando termina. Esto es útil para saber, por ejemplo, si el proceso hijo se ejecutó con éxito o si hubo algún error.

(parte 5) K:
Para poder orquestar los procesos que se crean, primero hay que agruparlos en dos categorias:
    primer plano:
        mismo gid que pid
    segundo plano:
        mismo gid en común al padre, distinto pid

La parte fundamental es que los procesos hijos cuando terminan mandan un evento (SIGCHLD) que lo resuelve un handler (background_handler) que previamente fue registrado (sigaction) para responder a ese evento particular, cuya funcion especifica es verificar si el proceso se encuentra terminado y si pertenece al grupo de "segundo plano" (mismo gid que la shell - padre).

---

### Flujo estándar

#### Investigar el significado de 2>&1, explicar cómo funciona su forma general y mostrar qué sucede con la salida de cat out.txt en el ejemplo. Luego repetirlo invertiendo el orden de las redirecciones. ¿Cambió algo?

El fragmento 2>&1 es utilizado en la shell para redirigir la salida de error estándar (stderr, que es el descriptor de archivo 2) al mismo lugar donde está siendo redirigida la salida estándar (stdout, que es el descriptor de archivo 1).

**Forma General**

La forma general es N>&M, donde N y M son descriptores de archivo. Esto significa "redirige el descriptor de archivo N al mismo lugar donde M está siendo redirigido".

Ejemplo 1 con cat `out.txt`: el comando ls -C /home /noexiste >out.txt 2>&1 intenta listar los archivos de los directorios /home y /noexiste en una sola columna. La salida exitosa se guarda en un archivo llamado out.txt gracias a >out.txt. Si el directorio /noexiste no existe, el mensaje de error también se guarda en out.txt porque 2>&1 redirige la salida de error al mismo lugar que la salida estándar. Así, tanto los resultados exitosos como los mensajes de error terminan en el archivo out.txt.

```
$ ls -C /home /noexiste >out.txt 2>&1
        Program: [ls -C /home /noexiste >out.txt 2>&1] exited, status: 2
 (/home/martina)
$ cat out.txt
ls: no se puede acceder a '/noexiste': No existe el archivo o el directorio
/home:
martina
        Program: [cat out.txt] exited, status: 0
```

Ejemplo 2 con `2>&1 >out1.txt`: El resultado no será el mismo. Aquí, 2>&1 redirigirá stderr a donde stdout está siendo redirigido, que en ese momento es la terminal porque la redirección > out.txt aún no ha tenido lugar. Luego, > out.txt redirige stdout al archivo out.txt, pero stderr seguirá yendo a la terminal.

```
 (/home/martina)
$ ls -C /home /noexiste 2>&1 >out.txt
ls: no se puede acceder a '/noexiste': No existe el archivo o el directorio
        Program: [ls -C /home /noexiste 2>&1 >out.txt] exited, status: 2
 (/home/martina)
$ cat out.txt
/home:
martina
        Program: [cat out.txt] exited, status: 0

```

¿Cambió algo al invertir el orden?
Sí, cambia. En el primer caso, tanto stdout como stderr se redirigen al archivo out.txt. En el segundo caso, solo stdout se redirige al archivo out.txt, mientras que stderr se muestra en la terminal.

---

### Tuberías múltiples

#### Investigar qué ocurre con el exit code reportado por la shell si se ejecuta un pipe ¿Cambia en algo? ¿Qué ocurre si, en un pipe, alguno de los comandos falla? Mostrar evidencia (e.g. salidas de terminal) de este comportamiento usando bash. Comparar con la implementación del este lab.

En Bash, el valor de salida (exit code) de un comando en pipeline es el valor de salida del último comando en el pipeline. Esto significa que si tienes un comando como cmd1 | cmd2 | cmd3, el valor de salida del pipeline completo será el valor de salida de cmd3.

¿Qué pasa si uno de los comandos falla?
Si uno de los comandos en el pipeline falla (es decir, tiene un valor de salida distinto de cero), los demás comandos del pipeline todavía se ejecutan. El valor de salida del pipeline completo seguirá siendo el valor de salida del último comando.

Ejemplo sin errores:

```
 (/home/martina)
$ ls -l | grep D
drwxr-xr-x 3 martina martina 12288 sep 14 21:10 Descargas
drwxr-xr-x 2 martina martina  4096 mar 12  2023 Documentos
```

En la implementación, si uno de los comandos en un pipeline falla, el pipeline completo sigue funcionando, al igual que en Bash. El manejo de errores y la continuidad del pipeline son consistentes con el comportamiento de Bash, lo que indica que la implementación es robusta en este aspecto.

Ejemplo:

```
$ ls /noexiste | wc -l
ls: no se puede acceder a '/noexiste': No existe el archivo o el directorio
0
```

---

### Variables de entorno temporarias

#### ¿Por qué es necesario hacerlo luego de la llamada a fork(2)?

Porque el alcance de las variables de entorno temporarias son sobre el proceso hijo. De realizarse el llamado previo al fork, esto afectaria tanto al proceso padre (shell) como al hijo.

#### En algunos de los wrappers de la familia de funciones de exec(3) (las que finalizan con la letra e), se les puede pasar un tercer argumento (o una lista de argumentos dependiendo del caso), con nuevas variables de entorno para la ejecución de ese proceso. Supongamos, entonces, que en vez de utilizar setenv(3) por cada una de las variables, se guardan en un arreglo y se lo coloca en el tercer argumento de una de las funciones de exec(3).

- ¿El comportamiento resultante es el mismo que en el primer caso? Explicar qué sucede y por qué.
- Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo.

Según la documentación de _exec(3)_, al realizar un llamado a alguna de estas funciones que contienen 'e' se utilizan exclusivamente las variables de entorno que se pasaron como argumento. En cambio, las que que no incluyen el sufijo 'e' utilizan las variables del proceso que padre.

Una posible implementación para que el comportamiento sea el mismo consiste en obtener las variables de entorno del proceso padre y pasarlas como argumento a, por ejemplo, `execvce`. Para esto se puede pasar directamente las variables del proceso padre (accedidas mediante `extern char ** environ`) o bien realizar una copia de estos valores, siempre conservando la condicion de que dicho array de cadenas de caracteres tenga `NULL` como último elemento.

### Pseudo-variables

#### Investigar al menos otras tres variables mágicas estándar, y describir su propósito.

Incluir un ejemplo de su uso en bash (u otra terminal similar).

- `$$` devuelve el PID que esta ejecutando el shell actual.

```console
$ echo $$
9254
```

- `$!` Es el PID del ultimo programa que ejecuto el shell en segundo plano.

```console
$ sleep 100 &
[1] 9247
$ echo $!
9247
```

- `$0` devuelve el nombre de la shell

```console
$ echo $0
bash
```

### Comandos built-in

#### ¿Entre cd y pwd, alguno de los dos se podría implementar sin necesidad de ser built-in? ¿Por qué?

En el caso de la instrucción cd (change directory), no sería posible implementarlo sin ser built-in, ya que se intentaría ejecutar como el resto de comandos, desde el proceso hijo. La problemática con esto es que al hacer el cambio al nuevo directorio, solo se vería aplicado en el proceso hijo y no para el caso del proceso padre.

Esto ocurre porque al hacer un fork ambos procesos apuntan a las mismas posiciones de memoria, pero al intentar hacer una modificación en alguna de estas, se duplica el espacio de memoria para que el cambio de un proceso no afecte en el otro.

Por otro lado, se podría proponer una implementación para pwd (print working directory), porque justamente no hace ningún cambio en las posiciones de memoria que están siendo usadas por el proceso.

#### ¿Si la respuesta es sí, cuál es el motivo, entonces, de hacerlo como built-in?

El principal motivo podría ser la eficiencia, ya que no es necesario ejecutar la syscall en un proceso aparte.

Además de esto, la acción de imprimir el directorio actual de la shell está directamente relacionado con el estado actual de la misma, por lo que resulta más apropiado que sea implementado built-in.

Por último, otro motivo por el que se hace built-in es que podría haber comportamientos no deseados al no hacerlo, un ejemplo de esto sería ejecutar el comando en segundo plano con "pwd &", suponiendo que se ejecute de manera casi instantanea otro comando, de cambio de directorio, podría llegar a imprimir el nuevo directorio.

#### Referencia

Para los planteos mencionados anteriormente nos basamos en la documentación de https://www.gnu.org/savannah-checkouts/gnu/bash/manual/bash.html#Shell-Builtin-Commands. En la sección "1.2 What is a shell?" da una breve explicación sobre esto.

---

### Historial

---
