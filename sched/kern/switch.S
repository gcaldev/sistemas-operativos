/*
 * Your code here for context_switch
 *
 * It should restore the register values in the Trapframe with the 'iret' instruction.
 * This exits the kernel and starts executing some environment's code.
 *
 * This function does not return.
 */

.globl context_switch;
context_switch:

	mov %esp,%eax /* guardo la direccion de memoria a la que apunta esp en eax*/
	addl $4,%eax  /*; incremento eax para dejar de apuntar al return address y apuntar al primer parametro */
	mov (%eax), %esp /* apunto a la direccion de memoria a la que apunta el puntero del struct */

	popal /* reestablezco los registros de proposito general */

	pop %es

	pop %ds

	addl $8,%esp  /* apunto a eip */

	iret
spin:
	jmp spin
