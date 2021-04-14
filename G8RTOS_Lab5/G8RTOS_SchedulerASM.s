; G8RTOS_SchedulerASM.s
; Holds all ASM functions needed for the scheduler
; Note: If you have an h file, do not have a C file and an S file of the same name

	; Functions Defined
	.def G8RTOS_Start, PendSV_Handler

	; Dependencies
	.ref CurrentlyRunningThread, G8RTOS_Scheduler

	.thumb		; Set to thumb mode
	.align 2	; Align by 2 bytes (thumb mode uses allignment by 2 or 4)
	.text		; Text section

; Need to have the address defined in file 
; (label needs to be close enough to asm code to be reached with PC relative addressing)
RunningPtr: .field CurrentlyRunningThread, 32

; G8RTOS_Start
;	Sets the first thread to be the currently running thread
;	Starts the currently running thread by setting Link Register to tcb's Program Counter
G8RTOS_Start:

	.asmfunc

	LDR R0, RunningPtr		; load currently running thread
	LDR R1, [R0]			; value of located at RunningPtr
	LDR SP, [R1]			; SP = RunningPtr->stack_ptr
	POP {R4 - R11}			; restore registers
	POP {R0 - R3}
	POP {R12}
	ADD SP, SP, #4			; discard LR from initial stack
	POP {LR}				; start location
	ADD SP, SP, #4			; discard PSR
	CPSIE I					; enable interrupts
	BX LR					; start first thread

	.endasmfunc

; PendSV_Handler
; - Performs a context switch in G8RTOS
; 	- Saves remaining registers into thread stack
;	- Saves current stack pointer to tcb
;	- Calls G8RTOS_Scheduler to get new tcb
;	- Set stack pointer to new stack pointer from new tcb
;	- Pops registers from thread stack
PendSV_Handler:
	
	.asmfunc
	CPSID I					; prevent interrupt during switch
	PUSH {R4 - R11}

	LDR R0, RunningPtr		; load value of RunningPtr (old thread)
	LDR R1, [R0]			; R1 = RunningPtr
	STR SP, [R1]			; save SP into TCB

	PUSH {R0, LR}
	; choose next thread to run
	BL G8RTOS_Scheduler
	POP {R0, LR}

	LDR R1, [R0]			; update to point to next thread
	LDR SP, [R1]

	POP {R4 - R11}

	CPSIE I
	BX LR
	.endasmfunc
	
	; end of the asm file
	.align
	.end
