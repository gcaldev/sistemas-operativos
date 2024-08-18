# sched


### Seguimiento GDB: Cambio de contexto
#### Previo a *context_switch* 

El estado de los registros, el valor del *trapframe*, su dirección y estado del stack se pueden ver en la siguientes imagenes, utilizando un *breakpoint* justo antes de invocar a *context_switch*
![Screenshot](/sched/gdb-screenshots/1a-pre-switch-status.png)
<p align="center">Valores de trapframe y registros previo a context_switch</p>

![Screenshot](/sched/gdb-screenshots/1b-pre-switch-stack.png)
<p align="center">Stack previo a invocar context_switch. El primer elemento es la direccion del trapframe (0xf02aa000) </p>

___
#### Dentro de *context_switch* 

Dentro de *context_switch* se puede ver que en el stack el primer elemento es la direccion para retornar normalmente al usar *call* (no será utilizada en este caso) y el segundo elemento es la dirección del *struct trapframe* provisto.
![Screenshot](/sched/gdb-screenshots/2-inside-switch.png)
<p align="center">Estado del stack al inicio de context_switch</p>

Se saltean 4 bytes para poder reapuntar el stack a la direccion del trapframe. Efectivamente se observa que los valores del stack coinciden con los valores esperados.

![Screenshot](/sched/gdb-screenshots/3a-sp-redirect.png)
<p align="center">esp (stack pointer) apuntando a trapframe (0xf02aa000)</p>

![Screenshot](/sched/gdb-screenshots/3b-sp-content.png)
<p align="center">Valores de stack una vez que apunta al trapframe</p>

___
#### Carga de registros 
Se cargan los registros. Al utilizar la instrucción `popa` se ovserva que los registros de uso general toman los valores provistos (todos en `0x00`)

![Screenshot](/sched/gdb-screenshots/4-popa.png)
<p align="center">Estado de registros antes y despues de usar la instruccion popa</p>


![Screenshot](/sched/gdb-screenshots/5-es-ds.png)
<p align="center">Estado antes y despues de cargar registros es y ds</p>

___
#### Uso de *iret*

Finalmente, debido a que *iret* requiere que en el stack estén los registros `eip` `cs` y `eflags`, se saltean dos palabras asociadas a `trapno` y `err`. Los tres primeros elementos coinciden con los valores esperados.
![Screenshot](/sched/gdb-screenshots/6-sp-skip-errno.png)
<p align="center">Estado de stack al saltear dos palabras</p>

![Screenshot](/sched/gdb-screenshots/7a-pre-iret.png)
<p align="center">Estado de registros inmediatamente antes de iret</p>

![Screenshot](/sched/gdb-screenshots/7-b-post-iret.png)
<p align="center">Estado de registros en instruccion cargada en el instruction pointer (eip = 0x800020) </p>



## Scheduler con prioridades

Se implementa un scheduler tipo *preemptive* con prioridades. Los criterios principales son
- La mayor prioridad (i.e. la mejor) se atribuye al número 0 y la menor (i.e. la peor) al número 100. 
- Se ejecuta el *environment* ejecutable de mayor prioridad posible: Si un proceso posee varios niveles de prioridad superior a otro, este último no se ejecutará entre los distintos *yields* hasta que la prioridad lo permita.
- Para que todos los *environments* puedan ejecutarse se disminuye la prioridad cada vez que el scheduler lo despacha. Esto permite que procesos de alta prioridad no se apropien de la CPU.
- Cada cierta cantidad de *yields* se ejecuta un mecanismo para mejorar la prioridad de *environments* de duracion considerable tal de evitar *starvation*. Se intenta evitar que todos los *environments*  mejorados tengan la misma prioridad y se comporten de tipo *Round Robin*  asignandoles prioridad en base a la paridad del indice del arreglo de *environments* tal de agregar un factor de variedad por azar. 
- Modificación de funciones/syscalls
  - Para el caso de *fork* se estable como criterio que el proceso hijo tenga una prioridad mínimamente inferior tal de mantener jerarquía de procesos. 
  - En el caso de `ipc_send` además de destrabar el *environment* que estaba bloqueado esperando se le asigna alta prioridad, tal de intentar compensar el bloqueo.

### Muestras de funcionamiento

#### Inicialización

Se modifica el archivo `kern/init.c` para agregar los siguientes procesos
- `idle`,con prioridad 0 (mejor prioridad posible) 
- `hello`, con prioridad 10
- `buggyhello` con prioridad 10
- `primes` con prioridad 30

cuyos ids de los procesos creados son 4096, 4097, 4098 y 4099 respecitvamente.

![Screenshot](/sched/scheduler-screenshots/1-init.png)
<p align="center">Procesos creados en inicializacion</p>


#### Ejecución de proceso prioritario

Dado que el proceso `idle` (4096) posee mayor prioridad se ejecuta exclusivamente en los primeros 10 *yields*.

![Screenshot](/sched/scheduler-screenshots/2-primeros-runs.png)
<p align="center">Ejecución prioritaria de idle</p>


#### Empate de prioridades

El proceso `idle` (4096) tiene su prioridad disminuida a medida que se ejecuta. Cuando su prioridad es igual a la de otros, se empieza a comportar como un *round robin*. En la figura se ve que a partir del décimo *yield* (`-YIELD_RUN (10)-`) comienza a despachar el 4096 (`idle`) seguido del 4097 (`hello`) y del 4098 (`buggyhello`) de forma ciclica.

Finalmente los procesos de `hello` y `buggyhello` terminan y solo se ejecuta `idle` ya que `primes` tiene menos prioridad.

![Screenshot](/sched/scheduler-screenshots/3-rr.png)
<p align="center">Ejecucion tipo Round Robin ante empate de prioridades</p>

#### Nuevo proceso

Una vez que la prioridad de `idle` disminuye y alcanza a `primes` se ejecuta este último. En la siguiente figura se observa que cuando este ejecuta su `fork` se crea un proceso nuevo hijo (8194) no ejecutable (la prioridad 0 es debido a que es la configuracion por defecto al alocar cualquier *enviroment*)

![Screenshot](/sched/scheduler-screenshots/4a-primes-new-proc.png)
<p align="center"> Creacion de nuevo *enviroments* dentro de primes (4099) </p>

En la siguiente figura se ve que `primes` (4099) completa el paso de `fork`, configurando al proceso hijo (8194) como ejecutable (*NOT_RUNNABLE* a *RUNNABLE*) y con su prioridad + 1, como se mencionó en los criterios generales.
![Screenshot](/sched/scheduler-screenshots/4b-primes-fork-set-runnable.png)
<p align="center"> Proceso hijo de primes (8194) pasa ser ejecutable con prioridad correspondiente. </p>

#### Prioridad especial ante casos bloqueantes

En el proceo de `primes` el hijo queda en estado bloqueado hasta que reciba datos del proceso padre. Como se mencionó previamente, al hacer esto el proceso bloqueado pasa a tener atención prioritaria al recibir datos. Se puede ver en la ejecución 48 a 49 del `yield`.
![Screenshot](/sched/scheduler-screenshots/5a-primes-not-runnable-a-runnable-prioritario.png)
<p align="center"> Proceso bloqueado 8194 pasa a tener maxima prioridad </p>

#### Manejo de *starvation*

Como se mencionó, para evitar potencial *starvation* cada 50 corridas del `yield` se intenta mejorar las prioridades de procesos longevos. Se ve que `idle` (4096) y `primes` (4099) pasan de 58 a 30 y de 59 a 10 respectivamente.

![Screenshot](/sched/scheduler-screenshots/6-starvation.png)
<p align="center"> Manejo de starvation </p>
