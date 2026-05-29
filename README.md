# Anura
Anura is a GUI debugger for my BSc Computer Science degree 3rd year dissertation  

## Demo video
https://jamestbest.co.uk/static/videos/diss_demo.mp4

## Targets
Anura is designed to be 'extensible' in that it should be possible to 'easily' add more target architectures and OSes.  
For this dissertation it will start by targeting Linux-x64.

## Features
Anura contains common debugger features; placing source/assembly breakpoints, step-into/out/over, disassembly, and stack traces. \
Anura's disassembly is facilitated by ISDL, the Instruction Set Description Langauge, this language allows writing the structure of the different instructions for e.g. x64, ARM32, etc. and can then generate a disassembler from this. The language is designed to be close to the format found in instruction set manuals. This is part of Anura's extensibility, allowing new target architectures to be more easily added with disassembly. ISDL is contained within [MtDoom/](MtDoom/) and the input format is [x64-format.txt](x64-format.txt), the output disassembler is within [x64/](x64/).
Anura contains two uncommon debugger features related to data analysis; namely break-on-cause and break-save. These features are designed to give insight into why some variable reached a target value, either by saving stack information that is destroyed, or by breaking at the earliest point in execution that it can guarentee the variable will later evaluate to the target value.

### Break-on-cause
Break on cause is the latter, given some target value for a variable it will use compiler information (new information, not currently emitted) to propagate new target value to the parts of code that affect the variable. For example, take the code below. Given that we want to find out why a == FAIL the target value is FAIL for variable a. The value of a depends on the result of f() which depends on the function f, which depends on two points, one being return success, another being return error. At the error point we branch the propagation as the return depends on both error() and a conditional on b and c, which in turn depend on two different readint()s. At runtime break-on-cause uses this dependancy tree along with some more information to decide for each point what the target value should be, and if that target value is reached if it can stop execution. The most complex part of this example is the conditional in f, here the compiler information can have the error path be constant, as it always returns FAIL. It then means that the targets move to the condition which is based on b and c, this can be moved to the line int b= readint() as at this point we have the value of c and so can propagate it to b's readint(). If we take readint to be an intrinsic for this example then it is at this line that we can decide if we should break, if the result is == c. This is actually done at the assembly level checking the value of registers or memory locations.

```c
int main() {
  int a= f();
  if (a == FAIL) return 1;

  return 0;
}

int f() {
  int c= readint();
  int b= readint();
  if (b == c) return error("Given invalid number");

  return SUCCESS;
}

int error(const char* message) {
  puts(message);
  return FAIL;
}
```

There is then a display once the condition is met, displaying the stack traces along with marker information. The main part is that the execution has paused at the earliest point and so the user can walk through the rest of the function and collect more information.

<img width="606" height="353" alt="image" src="https://github.com/user-attachments/assets/fefe8eba-89ad-4e23-948b-00a9b18fc17c" />


### break-save
Break-save is the simpler version of break-on-cause, it saves data and control flow points as well as stack frames, allowing the user to walk back through the programs execution. The compiler generates data and control flow points within the functions, as shown below. There is then a conditional breakpoint placed on the source line e.g. a == FAIL, and break-save will collect the points until the condition is met.

<img width="608" height="419" alt="image" src="https://github.com/user-attachments/assets/46ceafe3-7f68-43e1-bdb9-c5bad7a7a8c6" />

This data is then visualised to the user, as shown below, with the stack traces, and data/control flow points. Selecting them goes to the line in the source code. The value of the parameters and local variables is shown and as you walk through the control flow it updates based on the data points.

<img width="608" height="383" alt="image" src="https://github.com/user-attachments/assets/4ccc7995-0e4e-4060-946f-d6ce34a89027" />

## File names
The current file name scheme might be changed one day...  
GUI/TUI- Palantir, simple tui to handle input and queue actions
Control thread- IsildursBane, controls the target process through PTRACE  
ISDL- MtDoom
DWARF DIEs- DIE parsing is done by Balin 
ELF parser- Sauron  
DWARF parser- Saruman  
Logger- Tolkien
