#show title: set text(size: 30pt)
#show title: set align(center)
#title[
  Anura: A GUI Debugger
]

#grid(
  columns: (1fr),
  align(center)[
    James Coward \
    #link("mailto:psyjc25@nottingham.ac.uk")
  ],
)

#set heading(numbering: "1.1    ")

= Abstract

= Background
== What is a debugger
A debugger is a program designed to aid developers working on a program. Helping developers figure out what their program is doing at runtime. Logic errors are the largest cause of software bugs, syntax errors can be caught by compilers, but logic errors rely on code reviews and a deep understanding of how the code will execute. A debugger provides a view into the code, data, and control flow of a program, both statically, and dynamically as the program is run. It uses the debug information provided by compilers to show information such as variable names and types, a stack trace of the current execution, and a link between source code lines and assembly instruction lines for placing breakpoints.

== Debugging: A history
Debugging has evolved a

Some systems had hardware level debugging, for example the PDP-11 has a series of control switches that allowed for examining data at an address, poking data into an address, continuing executing, as well as single stepping an instruction or even single cycling the cpu #cite(<PDP11>). This information was all done through a series of switches on the peripheral of the computer. To examine data at an address for example you used the switch register which was 18-22 switches on the front and then switched on the LOAD switch that placed the value of the switch register into the bus address register, and then the EXAMine switch which read data at the bus address register.

When debugging on a machine with an operating system there has to be strong api support for controlling other processes and accessing the memory of other processes. 

todo:
With the advent of process protection in general purpose operating systems like UNIX and later Windows NT #cite(<WindowsVAddr>) 

Linux provides control of processes through their ptrace api #cite(<ptrace>) which allows for attaching to a process, continuing the process, single stepping the process, getting the general purpose register values, and accessing the data of the process. 

= Introduction
== Aims
The overall aim of this project is to develop an extensible GUI debugger that is independent of the current GDB and LLDB dominated backend choices. Providing users with an intuitive user interface that displays compiler generated debug information. Creating a backend that can be extended to support many different target operating systems and instruction sets.

todo: expand this
There are two sub-aims for the project. The first is to develop an instruction set disassembly language that can be used to generate disassemblers, and the second is to explore and create a demonstration of a data flow breakpoint which breaks the program at the earliest point that it can guarantee a variable will later evaluate to a certain value.  

== Motivation

== Objectives

== Related work
=== Related debuggers
=== Related ISDLs
=== Related break


== Linux-x64
=== ELF
On Linux the executable file format is ELF #cite(<ELFManual>). This format contains different sections, such as the text section for the program instructions, DWARF sections such as the debug_line for line number information, several string sections that contain debug strings, as well as eh_frame data which is used for frame information. Parsing the file requires first reading the header which contains information such as the version, entry point, file type, machine etc. @ELFManual[sec.~1-4]. The header also contains pointers to two tables, the program header table, and the program section table. 

==== Program headers
The program header table lists the different relocatable sections of the ELF and the link between their virtual address and location in the file. These are then placed in physical addresses later by the loader. @ExampleProgHdrs shows some example program headers. The virtual address of the entry point for this program is 0x1040, so the second LOAD segment contains the entry point.

#figure(caption: "Example part of program headers")[
  #table(columns: (1fr, 1fr, 1fr, 1fr),
    table.header([Type], [File offset], [Virtual addr], [RWX flags]),
    [PHDR], [0x0040], [0x0040], [R - -],
    [INTERP], [0x0318], [0x0318], [R - -],
    [LOAD], [0x0000], [0x0000], [R - -], 
    [LOAD], [0x1000], [0x1000], [R - E],
  )
  
]<ExampleProgHdrs>

==== Section headers
The program section table contains information on the aforementioned sections, the main parts are the name, type, and address. 

#figure(caption: "Example part of section headers")[
  #table(columns: (1fr, 1fr, 1fr),
    table.header([Name], [Type], [Address]),
    [.dynstr], [STRTAB], [0x0468],
    [.init], [PROGBITS], [0x1000],
    [.text], [PROGBITS], [0x1040],
    [.eh_frame], [PROGBITS], [0x20a0],
    [.data], [PROGBITS], [0x3000],
    [.debug_info], [PROGBITS], [0x311d]
  )
]

=== DWARF
DWARF is the debugging information format for ELF #cite(<DWARF5Manual>). This information contains different sections, in the debug_info section there are Debug Information Entries (DIEs) which contain information on the compilation units, the sub-programs within them, the variables, types, lexical blocks @DWARF5Manual[sec.~2.1]. DWARF also contains a line number program which stores information on converting the program addresses into source code lines, this is contained in the debug_line section @DWARF5Manual[sec.~6.2]. DWARF information also contains a debug_frame section which describes how to restore the registers of the previous frame at an address, this is used to unroll the stack trace @DWARF5Manual[sec.~6.4], debug_frame is often omitted and instead replaced with eh_frame which is used for exception handling and contains a very similar format to that of debug_frame with changes to the header @EHFrame. There are several sections that are used to store debug strings, for example debug_str, and debug_line_str, which are referenced by other debugging information.

=== x64

= Basic debugger
== Target abstraction
== Launching a target
== Control thread and actions
== ELF parsing
== DWARF DIE entries
== DWARF Line number program
== Frame information
== Breakpoints
== Stepping
== UI

= ISDL
== Motivation
== Design
== Changes through development
== Testing

= Break on cause
== Motivation
== Design
=== Compiler generated information
== Capturing frames
== Limitations
== Further development
== Examples

= Conclusion and reflections
== Project management
== Contributions
== Reflections
== Conclusion

= Bibliography


= Break on cause
A common format for the parsers I write contains code similar to that in [x], where there is a loop parsing top level statements that breaks early if there is an error in any of these. During the development of a recent parser I encountered an error in a statement that I hadn't yet put an error message for, which forced me to put a break on the top level function and continue through and count the number of successful and then rerun and stop before the erroring one. There are several solutions that could make this process easier; I could have placed a breakpoint on the if statement of the main loop and then inspected the value of calling current again to see where in the program I was, or I could .... However these felt like I was doing much of the set up instead of the debugger. 

This was enough for me to consider what I would want the debugger to actually do, what would be the ultimate help in this scenario, and in my mind it would be that it could break on the value that causes this early break. Breaking on the cause of the early failure which would be some return FAIL down the call stack.

The data flow in a program such as this is fairly simple, FAIL and SUCCESS values are typically in the lowest level of calling and then just propagate back up, and so I considered some compiler generated information that could describe the dependency of values, such as in this case res, allowing the debugger to place breakpoints in the required positions and then evaluate the value and decide if it can guarantee that something would occur later, which in this case is that res would equal FAIL.

The initial design described this as a dependency as in res depends on the expression parse_top_level() which depends on the function parse_top_level, which depends on four different return statements which depend on a conditional on c.type which depends on c which depends on consume() which depends on the function consume, etc.

There are two different types of flow to consider with this; control flow and data requirement flow. The control flow is important because the debugger needs to put breakpoints in positions before they run, but the data requirement can sometimes flow opposite to this. Consider the following example

```c
1: Token* c= current();
2: if (c == NULL) return FAIL;
```

The control flow of this section goes from line 1 to 2, and the debugger will need to place a breakpoint on line 1 before it reaches line 2. The data requirement flow however flows from line 2 to 1, if we are wanting to break on the cause of the value FAIL then the conditional c == NULL places the data requirement of c being NULL which then means that the return value of current() must also be NULL.



The conceptual process for the debugger is as follows. given some variable at a source line location with some target value, e.g. res = FAIL we walk the flow tree for that variable the first part is that it depends on the expression parse_top_level() so we place a breakpoint here and run the program, 


when we hit it we look at the target value, which is FAIL, we look at the location the compiler has placed this expression e.g. rax

Which requires a dependency list for each function and local variable in the codebase.


#columns(2)[
  ```c
  int parse() {
    ...
    while (stream_has_token()) {
      const int res= parse_top_statement();
      if (res == FAIL) return res;
    }
    return SUCCESS;
  }
  ```
  #colbreak()
  ```c
  int parse_top_statement() {
    Token* c= current();
    switch (c->type) {
      case TYPE_A: return parse_a();
      case TYPE_B: return parse_b();
      case TYPE_C: return parse_c();
    }
    return FAIL;
  }
  ```
]

= Frame information
Most debuggers provide some way of looking at the call stack, displaying the current frame and the function it's associated with and then recursively unwinding the stack to find where in the parent it was called from and where in the parent of the parent that was called from. This unwinding should also include restoring the value of the registers in the parent's frame which allows data in that frame to be viewed if it was overwritten and to be restored by the current frame.

A stack frame is an area of memory associated with the function instance, for the Linux x64 abi this contains the return address, the saved frame pointer, and then an area for local variables. It can also contain stack arguments when calling another function.  [https://refspecs.linuxbase.org/elf/x86_64-abi-0.99.pdf 3.2.2 The Stack Frame]


#figure(
  caption: [X64-Linux Stack Frame [https://refspecs.linuxbase.org/elf/x86_64-abi-0.99.pdf 3.2.2 The Stack Frame]],
  placement: none,
  {
    set table(align: (x, _) => if x == 0 { right } else {center})
    
    table(
      stroke: none,
      columns: (auto, 1fr, 1fr),
      table.header([Position], [Contents], [Frame]),
      table.vline(x: 1, start: 1),
      table.vline(x: 2, start: 1),
      table.hline(),
      [8n+16(%rbp)], [nth stack argument], table.cell(rowspan: 3, align: horizon)[Previous],
      [], [...],
      [16(%rbp)], [1st stack argument],
      table.hline(),
      [8(%rbp)], [Return address], table.cell(rowspan: 5, align: horizon)[Current],
      [0(%rbp)], [Previous frame pointer],
      table.hline(end: 2),
      [-8(%rbp)], table.cell(rowspan: 3, align: horizon)[Local data],
      [...],
      [0(%rsp)],
      table.hline(end: 2),
      table.hline()
    )
  }
)

As part of the x64 abi there are a number of registers that are listed as volatile and non-volatile, meaning they can be overwritten by the callee before use and that they must be preserved by the callee respectively. When unwinding the stack we want to restore the preserved registers as they might be used in a previous frame, or could be needed for the unwinding itself. Hence as part of the debugging information there is data on how to restore registers at different parts in the program. There are two different areas to find this information, firstly there may be a DWARF debug_line information entry, or more commonly an eh_frame entry. eh_frame data is there as part of the exception handling information for unwinding the stack when an exception occurs. They both follow very similar formats as eh_frame is based on the debug_line format. 

...

As mentioned before the start of the frame is marked by the frame base value, this could be in x64 the value of rbp, the base pointer, however the register may be omitted and it may instead be an offset from the stack pointer, rsp, which can change as the function progresses. These are abstracted in DWARF and referred to as the CFA, the canonical frame address. The first goal of the DWARF information is to record how to calculate the value of the CFA at the different addresses. Along with this is storing how to recover the value of the other registers, which often have their value at some offset from the CFA.

= Registers
One very standard part of a debugger is some way to view the contents of the registers, both the general purpose and more special case ones. In the Linux-x64 environment getting general purpose registers is very easy there is a ptrace action called GETREGS [x] which will copy into a struct which is exposed by 

= Disassembly
== Data in code
In some scenarios it may be the case that the compiler places data in the same area as the generated code. For example if the generated jump table for a switch statement is placed in the text section then when we disassemble we will eventually reach what is actually data and start decoding, which will produce garbage instructions that might extend over the real instructions that come after it, meaning all instructions after are incorrectly decoded. This is uncommon in modern compiler output as data like jump tables are more likely to be placed in read only sections of the binary, however it is interesting to consider how to solve this.

== Linear and Flow Disassembly
There are two main types of disassemblers; 

== Using DWARF information
We do however have more information that a disassembler that is working on a binary without debug information, and that is we have a mapping of the source line numbers to their instruction address. This means we have known locations that are the start of instructions, and so even if it's the case that one line inserts data we can decode up to it's end point and then if that instruction has decoded past when it should it won't affect the next instruction as we know which pc value it starts at.

Consider if the information we had was as follows:

Assembly:
#table(
  stroke: none,
  columns: (auto, auto, auto, auto),
  [], [0x1100], [```nasm mov rbx, QWORD PTR [switch_data + rdi*8]```], [48 8b 1c fd 08 11 00 00],
  [], [0x1108], [```nasm jmp jmp_label ```], [e9 25 00 00 00],
  [```nasm switch_data:```], [0x110D], [```nasm .long 0xe8010203```], [e8 01 02 03],
  [], [0x1115], [```nasm .long 0x04e80607```], [04 e8 06 07],
  [], [0x111D], [```nasm.long 0x0809e811```], [08 09 e8 11],
  [], [0x1125], [```nasm.long 0x121314e8```], [12 13 14 e8],
  [```nasm jmp_label:```], [0x112D], [```nasm mov rax, rbx```], [48 89 d8],
  [], [0x1135], [```nasm ret```], [c3]
)

For a linear disassembler it would decode the move and jump instructions and then continue into the data where it would decode the byte e8, which is the call instruction and then read 4 bytes for the location, giving call 0x4030206 (the displacement of e8 is relative to the next instruction so after the 5 bytes of this instruction hence the 06 over 01 [Intel® 64 and IA-32 Architectures Software Developer’s Manual Volume 2, Vol. 2A 3-121, CALL—Call Procedure]). This would then leave the stream looking at the next byte which is another e8. and the process repeats until we reach the last byte of data which is another e8, except this time there is no more data to read as the displacement and so it reads the bytes of the move and return instruction decoding to call 0xc3d8894d.
```nasm
mov rbx, QWORD PTR [0x110D + rdi*8]
jmp 0x112D
call 0x4030206
call 0x9080710
call 0x14131220
call 0xc3d8895c
```
For a flow disassembler this is not a concern in this instance as it would follow the jmp control flow to decode at 0x112D skipping all the data. A case where flow disassemblers fail is if the control flow has been maliciously crafted to pretend that it might go into the data when in reality it always skips it. An example of this is shown in [code 1]

```nasm
xor rax, rax
cmp rax, 0
jz label
```

Although the DWARF information wouldn't be susceptible to this kind of control flow it would require the scenario in which anti-disassembly has been used while still leaving debugging information available, which is not likely. It is also the case that unlike a flow disassembler using the DWARF info would still decode the data as instructions it's only the case that it can recreate the actual instructions.
```c
int main(int argc) {
  long a;
  switch {
    case 0: a= 0xe8010203; break;
    case 1: a= 0x04e80607; break;
    case 2: a= 0x0809e811; break;
    case 3: a= 0x121314e8; break;
  }
  return a;
}
```

Line mapping:
#table(
  columns: (auto, 1fr, 1fr),
  [Line number], [Start address], [End address (Exclusive)],
  [3], [0x1100], [0x112D],
  [9], [0x112D], [0x1136]
)

With these mappings we can have the disassembler start decoding at 0x1100 until it reaches 0x112D at which point we start anew from the next source line entry which is 0x112D. The result of which is shown below. We can mark a decoded line as an error or invalid based on if it decodes outside the end address of the line. This will only work on the last instruction that is actually data only if it requires more bytes than are left in the range.

```nasm
mov rbx, QWORD PTR [0x110D + rdi*8]
jmp 0x112D
call 0x4030206
call 0x9080710
call 0x14131220
call 0xc3d8895c # Marked: overflows by 4 bytes
mov rax, rbx
ret
```

== 



= Breakpoints
There are two types of breakpoint implementation; hardware and software. In x64 there are 8 hardware registers associated with debugging. Two are reserved being DR4 and DR5. DR7 is the debug control register and controls which breakpoint registers are active, the type and length of the breakpoint, as well as two flags. DR6 is the debug status register which contains information on the last debug exception that was generated, for example a bit for each of the hardware registers to specify if they were hit, as well as a single step flag for if the instruction was single stepped. Debug registers DR0-3 contain the addresses to break on.

Outside of implementation there are 


== Software breakpoints
Software breakpoints involve placing some instruction that will cause an exception to generate when the processor reaches it. These can be tailored instructions like x64's INT3 [x] which generates a breakpoint exception when hit, or by placing some illegal/invalid instruction that will generate an exception. The debugger receives all signals for the child process first and so can intercept these exceptions and choose how to handle them. Placing a software breakpoint requires storing what the original byte(s) at the location were to be replaced by the new instruction, I'll refer to this as the shadow. X64's hardware instructions generate the exception before the instruction is executed and so the instruction pointer points to the address, whereas for software breakpoints the INT3 instruction has been executed to generate the exception and so the instruction pointer is one ahead, . In order to continue execution for any reason, be it single stepping, or full running, we have to replace the original instruction so that it can be executed. This requires placing the shadow back, single stepping, and then replacing the trap instruction again.


#let isdl(code)= raw(
  code,
  lang: "ISDL",
  syntaxes: "isdl.sublime-syntax",
  theme: auto
)

= ISDL
The Instruction Set Description Language is a language designed to generate disassemblers.

== #isdl("MAP") statement
It is very common for x64 to take bits of information from different sections of the instruction and combine them to represent something. This mainly occurs with the registers which can be extended by bits the REX prefix byte to form a 4 bit mapping to registers. The majority of the bits for registers are combined with the opcode byte or in the ModRM byte.

For example take a ModRM byte of 11 101 000, and REX byte of 0100 .w=0 .r=1 .x=0 .b=0. The register encoded in the ModRM's register part is made up of both the 3 bits in the ModRM byte and the 1 bit REX.r field. The full 4 bit encoding is 1101, which encodes R13.

The description language therefore requires some way of combining bits of information. This could be done by creating an alias as follows

#isdl(
"ALIAS reg_with_r= {
   if (REX.r) {
      000= \"R8W\", \"R8B\", \"R8B\", \"R8D\", \"R8\"
      ...
      101= \"R13W\", \"R13B\", \"R13B\", \"R13D\", \"R13\"
   }
   000= \"AX\", \"AL\", \"AL\", \"EAX\", \"RAX\"
   101= \"BP\", \"CH\", \"CH\", \"EBP\", \"RBP\"
}
")

This works however, consider if we need to combine the register bits with a different bit. For example REX.b is combined with the 3 bits in the SIB byte. There would then be another alias with all the registers written out again. In fact we need one for the REX's r,b, and x fields.

This lead to the introduction of the #isdl("MAP") statement. The general structure of which is

#isdl("MAP <dst_alias> <input0> <input1> ...")

This allows instances of different registers that combine the needed bit information and pass it into another alias as the input stream.

#isdl("
DATA regx 3 BITS
ALIAS regr 3 BITS = {
    regx = MAP regO REX.r regx
}
ALIAS regop 3 bits = {
    regx = MAP regO REX.b regx
}")

The above statements mean read in regx which is a 3 bit data field and then create an input stream with the bit REX.b followed by the 3 bits of regx and get the output of regO with that input stream.

regO is an instance of reg which is simply a mapping of 4 bit inputs into the register strings.

#isdl("
ALIAS reg 4 BITS = {
  0000= \"AX\", \"AL\", \"AL\", \"EAX\", \"RAX\"
  ...
  1101= \"R13W\", \"R13B\", \"R13B\", \"R13D\", \"R13\"
}
ALIAS regO= reg
")

== #isdl("ALIAS") statement
The alias statement is the main part of the language, there are two types of aliases, the first is the standard alias which contains a list of rules. When the alias is in the left side of another rule it goes through its rules in sequence until one matches. These rules can be left rules, left-right rules, if rules, or with rules. Left rules are simply mean try parsing this alias, an example of this is the alias for operations.

#isdl("ALIAS op= {
  PUSH
  SUB
  HALT
  CALL
}")



== #isdl("WITH") and #isdl("VAR") statements
When finishing the original version of ISDL it became clear that one assumption was a problem, that being the calculation of operand size. For a lot of instructions the opmode's size i.e. if it is 8/16/32/64 bits uses the following

#isdl("CALCULATE opmode= {
  if ow then 8bit
  if REX.w then 64bit
  if lp3? then 16bit
  32bit
}")

where 32 bits is the default size. This is not true for all instructions however, and in 64 bit long mode some operations like PUSH have certain opcodes that have a 64 bit default operand size. This is a problem because in the original language design opmode is used throughout the language to decide if a register should decode as rax or eax, or if a 16 bit immediate should be read or a 32 bit, and choosing which pointer string should be used e.g. DWORD PTR or QWORD PTR. One fix for this would be to create two version of these registers, ModRM byte, immediates etc. however this would be tantamount to writing the language specification twice. The language therefore needed some way of changing data but only for the current instruction line.

Two statements were introduced to fix this. Firstly is the VAR or variable statement. These are extensions to the #isdl("FLAG") statements with a structure of

#isdl("VAR <var_name> OF <flag_name> = <flag_value>")

This allowed variables to be used in place of normal flags, which includes the #isdl("CALCULATE") statement.

#isdl("CALCULATE opmode= {
  if ow then 8bit
  if REX.w then 64bit
  if lp3? then 16bit
  default_opmode
}")

The second part is to allow instructions to change this variable when decoding, this is done through #isdl("WITH") statements. 

#isdl("WITH <var_name> = <flag_value> {
  <rule>
  <rule>
}")

#isdl("ALIAS PUSH= {
  WITH default_opmode = 64bit {
    0xFF ModRM_6 = \"PUSH {ModRM_6}\"
    0101 0 regop = \"PUSH {regop}\"
  }
  0110 10 ow 0 immM32 = \"PUSH {immM32}\"
  0x0F 0xA0 = \"PUSH FS\"
  0x0F 0xA8 = \"PUSH GS\"
}
")

== #isdl("STRUCTURE") statement
The structure statement describes the top level structure of the instruction set and is allowed to use some special operators. 


The x64 structure statement is shown below, the special operators are the same as in regex, where '\*' allows for any number of instances of the identifier, including zero. '?' allows for one or no instances.

#isdl("STRUCTURE lprefix* prefix? op = op")


== ISDL improvements
=== Optimisation
Currently in ISDL there are no optimisations, one impactful one would be improving how the correct instruction is selected. In the unoptimized version this is done sequentially with the opcode tested each time. To improve this would require having a sorted list of the first byte and being able to binary search through based on the opcode. There are two problems to this, one is that not all opcodes are an entire byte which I'll discuss later, and second is that the opcode value is hidden behind aliases as seen in the operator alias [x]. To fix this ISDL could propagate up constants at the start of a rule, if done until all constants are completely propagated the operation alias would look like

#isdl("ALIAS op= {
  0xFF PUSH_000 = PUSH_000
  0x83 SUB_000 = SUB_000
  0xF4 HALT_000 = HALT_000
  0xE8 CALL_000 = CALL_000
  0xFF CALL_001 = CALL_001
}")

This would then allow sorting on the byte and jumping to the correct start without having to go linearly through. As alluded to before not all opcodes start with a unique one byte opcode, and with the way ISDL is designed some contain information e.g. the ow bit found in a lot of instructions decides if the operand is 8 bit, and instead of writing two instruction lines with virtually the same information it is #isdl("DATA") that is used when calculating the opmode. This means that instructions often appear in the form #isdl("`1100 011 ~ow`") or #isdl("`1011 ~ow regop`"). Instead of requiring the constant to be an entire byte it would instead take all constant bits and then create an if-else binary tree with the if being the next bit is a 1 and the else a 0.

#isdl("ALIAS op= {
  00001111 10100000 PUSH_003 = PUSH_003
  00001111 10101000 PUSH_004 = PUSH_004
  0010110 SUB_001 = SUB_001
  01010 PUSH_001 = PUSH_001
  011010 PUSH_002 = PUSH_002
  1000000 SUB_002 = SUB_002
  10000011 SUB_000 = SUB_000
  11101000 CALL_000 = CALL_000
  11110100 HALT_000 = HALT_000
  11111111 PUSH_000 = PUSH_000
  11111111 CALL_001 = CALL_001
}")

As you can see from this small snippet of instructions we already eliminate around half of the instructions each time we read a new bit.

=== Design
ISDL is extremely generic, it is simply a way of converting binary into formatted string, this makes it quite powerful and potentially useful in other areas. However, it does also mean that it is not as powerful as some disassemblers, mainly in regards to formatted information on the instruction. ISDL doesn't have any way of outputting the opcode of the instruction or the operand(s), it cannot show the conditional code used outside of the instruction mnemonic. It is possible to find the instruction size but only given the fact that the stream has moved a certain number of bytes from the start and end of disassembling.

== Data size vs Address size
In x64 there are two different sizes that get encoded, the first is operand size, these are for immediates and registers. There is also the address size this is for registers that are being used as . The default operand size is 32 bits whereas the default address size is 64 bits. As stated before we don't want to have to write out the entire register bank again with a new rule set, so ISDL allows aliases to keep their multi-valueness until an alias with rules is used. 

We can then create a new alias for registers being used in a memory context. Giving them a new right rule

#isdl("ALIAS regM= reg")

#isdl("RULE RIGHT ON regM {
  CHOOSE 0 if addrmode == 16bit
  CHOOSE 1 if 0
  CHOOSE 2 if 0
  CHOOSE 3 if addrmode == 32bit
  CHOOSE 4 if addrmode == 64bit
}")

This allows for decoding the following ```nasm MOV DWORD PTR [RBP + 0xfc], 0x1``` where the operand size is 32 bits as shown by the ```nasm DWORD PTR``` but the register being used as the base is rbp, the 64 bit variant of the stack base register.

== Reading data
There are two parts of reading data that required special consideration in the language, the first is that in x64 sometimes an instruction bit can mean the inverse of how it's usually used. For example the previously mentioned ow bit has the meaning \`The operand is 8 bit if ow is set\` in some instructions and \`The operand is 8 bit if ow is not set\` in others, but the ow bit is used the same when calculating the opmode. This is done via the `~` operator which reads data inverted. The second consideration is endianness, specifically when reading instruction data. In x64 data is stored in little endian [x] which means that the lowest order byte is stored at the lowest address in memory. Consider the data `0x12 0x34 0x45 0x67` in an instruction, the actual value of this is 0x67453412. ISDL contains a #isdl("META") statement in which the endianness can be specified, then when reading multi-byte data the byte is placed in the correct location by shifting.

#bibliography("biblo.bib")