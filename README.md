# Anura
Anura is a GUI debugger for my BSc Computer Science degree 3rd year dissertation  

## Targets
Anura is designed to be 'extensible' in that it should be possible to 'easily' add more target architectures and OSes.  
For this dissertation it will target Linux-x64

## Layout
Anura currently has two parts; the TUI (one day to be GUI) and control thread.  
The UI queues actions from the user, for example setting breakpoints, continuing the program, listing information  
This information however can only be enacted by the control thread who blocks on the target program, so the actions are
queued and then once the target is in a PTRACE_STOPPED state can it go through the actions.  

## File names
The current file name scheme might be changed one day...  
TUI- Palantir, simple tui to handle input and queue actions  
Control thread- IsildursBane, controls the target process through PTRACE  
ELF parser- Sauron  
DWARF parser- Saruman  
Logger- Tolkien, Warning; this is currently NOT atomic, I may add a lock- not sure  