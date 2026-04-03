int main() {
	asm volatile(
        	"jmp 1f\n"                  // skip over data
        ".loc 1 0 0\n"        	
".quad 0x1122334455667788\n"
        	".quad 0xdeadbeefcafebabe\n"
        	"1:\n"
	);


	return 0;
}
