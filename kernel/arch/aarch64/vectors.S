// Tell nvim that this is aarch64 assembly instead of vmasm
// vim: ft=arm64asm

// Macro for pushing all registers (at least those that are important) to the stack
.macro pushall
	mov x0, sp
	sub x0, x0, #296
	str x1, [x0, #8]
	stp x2, x3, [x0, #16]
	stp x4, x5, [x0, #32]
	stp x6, x7, [x0, #48]
	stp x8, x9, [x0, #64]
	stp x10, x11, [x0, #80]
	stp x12, x13, [x0, #96]
	stp x14, x15, [x0, #112]
	stp x16, x17, [x0, #128]
	stp x18, x19, [x0, #144]
	stp x20, x21, [x0, #160]
	stp x22, x23, [x0, #176]
	stp x24, x25, [x0, #192]
	stp x26, x27, [x0, #208]
	stp x28, x29, [x0, #224]
	str x30, [x0, #240]
	mov sp, x0
.endm

.macro	popall
        mov x0, sp
        ldr x1, [x0, #8]
        ldp x2, x3, [x0, #16]
	ldp x4, x5, [x0, #32]
	ldp x6, x7, [x0, #48]
	ldp x8, x9, [x0, #64]
	ldp x10, x11, [x0, #80]
	ldp x12, x13, [x0, #96]
	ldp x14, x15, [x0, #112]
	ldp x16, x17, [x0, #128]
	ldp x18, x19, [x0, #144]
	ldp x20, x21, [x0, #160]
	ldp x22, x23, [x0, #176]
	ldp x24, x25, [x0, #192]
	ldp x26, x27, [x0, #208]
	ldp x28, x29, [x0, #224]
	ldr x30, [x0, #240]
	add x0, x0, #296
	mov sp, x0
.endm

.macro make_handler name, el, callback
\name :
  // Load stack into x0 (original x0 is saved into tpidrro_el0)
  msr tpidrro_el0, x0
  ldr x0, =__stack_high
  mov sp, x0

  pushall

  // Save old stack pointer
  .if \el == 0
  mrs x1, sp_el0
  .else
  mov x1, sp
  .endif
  str x1, [sp, #248]

  // Save some important stuff
  mrs x0, elr_el1
  str x0, [sp, #256]
  mrs x0, spsr_el1
  str x0, [sp, #264]
  mrs x0, esr_el1
  str x0, [sp, #272]
  mrs x0, far_el1
  str x0, [sp, #280]
  
  // Finnally, save x0
  mrs x0, tpidrro_el0
  str x0, [sp, #0]

  // Call the handler
  mov x1, \el         // Pass the EL
  mov x0, sp          // Pass the stack pointer
  bl \callback
  wfi

  // Restore x0
  ldr x0, [sp, #0]
  msr tpidrro_el0, x0

  // Restore context
  ldr x0, [sp, #280]
  msr far_el1, x0
  ldr x0, [sp, #272]
  msr esr_el1, x0
  ldr x0, [sp, #264]
  msr spsr_el1, x0
  ldr x0, [sp, #256]
  msr elr_el1, x0

  // Restore stack pointer
  ldr x0, [sp, #248]
  .if \el == 0
  msr sp_el0, x0
  .else
  mov sp, x0
  .endif

  popall
  mrs x0, tpidrro_el0
  eret

.endm

// A simple macro for making vector table entries
.macro    make_entry  label
.align    7
b    \label
.endm

// The interrupt handler table
.align 11
.globl vectors
vectors:
  // EL1 Interrupt with EL0.SP == EL1.SP
  make_entry	el1t_sync			// Synchronous
  make_entry	el1t_irq			// IRQ
  make_entry	el1t_fiq			// FIQ
  make_entry	el1t_error			// Error

  // EL1 Interrupt with EL0.SP != EL1.SP
  make_entry    el1h_sync			// Synchronous
  make_entry	el1h_irq			// IRQ
  make_entry	el1h_fiq			// FIQ
  make_entry	el1h_error			// Error

  // EL0 Interrupt with cpu in Aarch64 mode
  make_entry	el0_sync			// Synchronous
  make_entry	el0_irq			        // IRQ
  make_entry	el0_fiq      		        // FIQ
  make_entry    el0_error			// Error

  // EL0 Interrupt with cpu in Aarch32 mode
  // NOTE: These Interrupts are ignored in the handler since mrk runs only pure aarch64 code
  make_entry	not_allowed			// Synchronous
  make_entry	not_allowed			// IRQ
  make_entry	not_allowed			// FIQ
  make_entry	not_allowed			// Error

// Just return for now...
not_allowed:
  wfi

make_handler    el1t_sync,  1, handle_intr 
make_handler    el1t_irq,   1, handle_irq                    
make_handler    el1t_fiq,   1, handle_irq                     
make_handler    el1t_error, 1                     

make_handler    el1h_sync,  1, handle_intr                    
make_handler    el1h_irq,   1, handle_irq                   
make_handler    el1h_fiq,   1, handle_irq                        
make_handler    el1h_error, 1, handle_intr           

make_handler    el0_sync,   0, handle_intr                   
make_handler    el0_irq,    0, handle_irq
make_handler    el0_fiq,    0, handle_irq                   
make_handler    el0_error,  0, handle_intr                   


// Loads vbar_el1 with our interrupt table
.globl __load_vectors
__load_vectors:
  ldr	x0, =vectors		
  msr	vbar_el1, x0	
  ret

