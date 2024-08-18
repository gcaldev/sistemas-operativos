#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>
#include <kern/env.h>

#define HISTORY_SIZE 50
#define ENV_ID_NOT_PRESENT -1

#define ENABLE_PRINT_DEBUG

#ifdef ENABLE_PRINT_DEBUG
#define PRINT_DEBUG(...) cprintf(__VA_ARGS__);
#else
#define PRINT_DEBUG(...)  // No hacer nada
#endif

void sched_halt(void);

static int last_index[NCPU];

int history[HISTORY_SIZE];

static int last_history_index = 0;

static int sched_calls_count = 0;

void
history_init()
{
	for (size_t i = 0; i < HISTORY_SIZE; i++) {
		history[i] = ENV_ID_NOT_PRESENT;
	}
}

static void
history_print()
{
	cprintf("Historial de procesos ejecutados [más a menos reciente]\n");
	for (int i = HISTORY_SIZE; i >= 0; i--) {
		int index = (last_history_index + i) % HISTORY_SIZE;

		if (history[index] != ENV_ID_NOT_PRESENT)
			cprintf("env_id: %ld\n", history[index]);
	}
}

static void
print_envs_info()
{
	cprintf("Información por proceso: \n");
	for (size_t i = 0; i < NENV; i++) {
		if (envs[i].env_id != 0)
			cprintf("ID: %ld	EJECUCIONES: %ld	"
			        "PRIORIDAD FINAL: %ld\n",
			        envs[i].env_id,
			        envs[i].env_runs,
			        envs[i].env_priority);
	}
}

static void
print_statistics()
{
	print_envs_info();
	history_print();
	cprintf("Cantidad de llamadas al scheduler: %d\n", sched_calls_count);
}

static void
sched_rr(void)
{
	int cur_cpu = cpunum();  // Obtener el número de la CPU actual
	int start_index = last_index[cur_cpu];

	for (int i = 0; i < NENV; i++) {
		int index = (start_index + i) % NENV;
		if (envs[index].env_status == ENV_RUNNABLE) {
			last_index[cur_cpu] = (index + 1) % NENV;
			env_run(&envs[index]);
			return;
		}
	}

	// No hay ningún entorno ejecutable.
	if (curenv && (curenv->env_status == ENV_RUNNING ||
	               curenv->env_status == ENV_RUNNABLE)) {
		env_run(curenv);
		return;
	}

	sched_halt();
}

/*
 * Mecanismo para starvation
 * A los procesos con baja prioridad que superen cierto umbral se les reasigna
 * una prioridad mejor para evitar starvation.
 *
 * Para agregar variedad de prioridades con cierta aleatoreidad
 * se les asigna un valor distinto a los environments segun la paridad
 * del indice del arreglo de environments.
 * Esta paridad se invierte en cada llamado a esta funcion.
 */
static void
handle_starved_enviroments()
{
	static int parity_favor = 0;

	parity_favor = !parity_favor;  // Toggle entre llamados

	PRINT_DEBUG("Handle starve (parity: %d)\n", parity_favor);

	for (int i = 0; i < NENV; i++) {
		struct Env *env = &envs[i];

		int status = env->env_status;
		bool is_valid_status =
		        (status == ENV_RUNNABLE || status == ENV_RUNNING ||
		         status == ENV_NOT_RUNNABLE);

		if (env->env_priority < 50 || !is_valid_status)
			continue;

		if (i % 2 == parity_favor) {
			env->env_priority = 10;
		} else {
			env->env_priority = 30;
		}

		PRINT_DEBUG("(idx:%d) ID: %d, Nueva prioridad: %d\n",
		            i,
		            env->env_id,
		            env->env_priority)
	}

	PRINT_DEBUG("Handle starve end\n")
}


void
sched_pr(void)
{
#ifdef ENABLE_PRINT_DEBUG
	static uint32_t yield_runs = 0;
	yield_runs++;

	PRINT_DEBUG("-----------------YIELD RUN (%d)-----------------\n",
	            yield_runs)
#endif

	sched_calls_count++;
	int cur_cpu = cpunum();  // Obtener el número de la CPU actual
	int start_index = last_index[cur_cpu];

	static int sched_starv_counter = 0;
	sched_starv_counter++;
	if (sched_starv_counter >= 50) {
		handle_starved_enviroments();
		sched_starv_counter = 0;
	}

	struct Env *best = NULL;

	for (int i = 0; i < NENV; i++) {
		int index = (start_index + i) % NENV;

		if (envs[index].env_status == ENV_RUNNABLE) {
			if (best == NULL) {
				best = &envs[index];
				last_index[cur_cpu] = index;
				PRINT_DEBUG("(RUNNABLE) Primer ENV encontrado: "
				            "id: %d, prioridad: %d\n",
				            best->env_id,
				            best->env_priority)
				continue;
			}

			if (envs[index].env_priority < best->env_priority) {
				best = &envs[index];
				last_index[cur_cpu] = index;
				PRINT_DEBUG("(RUNNABLE) Mejor prioridad -> "
				            "Nuevo ENV id: %d, priority: %d\n",
				            best->env_id,
				            best->env_priority)
			} else {
				PRINT_DEBUG(
				        "(RUNNABLE) Descartado ENV id: %d, "
				        "priority: %d por menor prioridad "
				        "(actual mejor id: %d priority: %d)\n",
				        envs[index].env_id,
				        envs[index].env_priority,
				        best->env_id,
				        best->env_priority)
			}
		} else if (envs[index].env_status == ENV_NOT_RUNNABLE) {
			PRINT_DEBUG("(NOT RUNNABLE) id: %d, priority: %d\n",
			            envs[index].env_id,
			            envs[index].env_priority)
		} else if (envs[index].env_status == ENV_RUNNING) {
			PRINT_DEBUG("(RUNNING) id: %d, priority: %d\n",
			            envs[index].env_id,
			            envs[index].env_priority)
		}
	}

	if (best != NULL) {
		// Contemplar current y que puede no estar corriendo (e.g. block desde syscall)
		if (curenv && curenv->env_status != ENV_NOT_RUNNABLE &&
		    (curenv->env_priority < best->env_priority)) {
			best = curenv;
		}

		history[last_history_index] = best->env_id;
		last_history_index = (last_history_index + 1) % HISTORY_SIZE;

		PRINT_DEBUG(">>> SCHEDULER RUN: id: %d (priority: %d)\n",
		            best->env_id,
		            best->env_priority)
		if (best->env_priority < 100) {
			best->env_priority = best->env_priority + 1;
		}
		env_run(best);
		return;
	}

	// No hay ningún entorno ejecutable.
	if (curenv && (curenv->env_status == ENV_RUNNING)) {
		history[last_history_index] = curenv->env_id;
		last_history_index = (last_history_index + 1) % HISTORY_SIZE;

		PRINT_DEBUG(
		        ">>> SCHEDULER RUN (curenv): id: %d (priority: %d)\n",
		        curenv->env_id,
		        curenv->env_priority)
		env_run(curenv);
		return;
	}

	sched_halt();
}

void
sched_yield(void)
{
#ifdef SCHED_RR
	sched_rr();
#endif
#ifdef SCHED_PR
	sched_pr();
#endif
}


// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		print_statistics();
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Once the scheduler has finishied it's work, print statistics on
	// performance. Your code here

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile("movl $0, %%ebp\n"
	             "movl %0, %%esp\n"
	             "pushl $0\n"
	             "pushl $0\n"
	             "sti\n"
	             "1:\n"
	             "hlt\n"
	             "jmp 1b\n"
	             :
	             : "a"(thiscpu->cpu_ts.ts_esp0));
}
