//
// Created by jamestbest on 9/27/25.
//

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <elf.h>

#include "Sauron.h"

bool verify_special(const uint8_t* start) {
    return memcmp(start, ELFMAG, SELFMAG) == 0;
}

const char* EI_CLASS_STR[ELFCLASSNUM]= {
        [ELFCLASSNONE]= "<<ERROR>> INVALID CLASS <</ERROR>>",
        [ELFCLASS32]= "32 bit",
        [ELFCLASS64]= "64 bit",
};

const char* EI_DATA_STR[ELFDATANUM]= {
        [ELFDATANONE]= "<<ERROR>> INVALID DATA ENCODING <</ERROR>>",
        [ELFDATA2LSB]= "2's compliment, little endian",
        [ELFDATA2MSB]= "2's compliment, big endian"
};

const char* get_osabi_str(uint8_t os_abi_id) {
    switch (os_abi_id) {
        case ELFOSABI_SYSV: return "Unix System V"; // alias _NONE
        case ELFOSABI_HPUX: return "HP-UX";
        case ELFOSABI_NETBSD: return "NetBSD";
        case ELFOSABI_LINUX: return "LINUX"; // alias _GNU
        case ELFOSABI_SOLARIS: return "Sun Solaris";
        case ELFOSABI_AIX: return "IBM AIX";
        case ELFOSABI_IRIX: return "SGI Irix";
        case ELFOSABI_FREEBSD: return "FreeBSD";
        case ELFOSABI_TRU64: return "Compaq TRU64 UNIX";
        case ELFOSABI_MODESTO: return "Novell Modesto";
        case ELFOSABI_OPENBSD: return "OpenBSD";
        case ELFOSABI_ARM_AEABI: return "ARM EABI";
        case ELFOSABI_ARM: return "ARM";
        case ELFOSABI_STANDALONE: return "Standalone (embedded) application";

        default:
            return "<<ERROR>> INVALID or UNKNOWN os abi <</ERROR>>";
    }
}

const char* get_em_str(unsigned int id) {
    switch (id) {
        case EM_NONE: return "No machine";
        case EM_M32: return "AT&T WE 32100";
        case EM_SPARC: return "SUN SPARC";
        case EM_386: return "Intel 80386";
        case EM_68K: return "Motorola m68k family";
        case EM_88K: return "Motorola m88k family";
        case EM_IAMCU: return "Intel MCU";
        case EM_860: return "Intel 80860";
        case EM_MIPS: return "MIPS R3000 big-endian";
        case EM_S370: return "IBM System/370";
        case EM_MIPS_RS3_LE: return "MIPS R3000 little-endian";
        case EM_PARISC: return "HPPA";
        case EM_VPP500: return "Fujitsu VPP500";
        case EM_SPARC32PLUS: return "Sun's \"v8plus\"";
        case EM_960: return "Intel 80960";
        case EM_PPC: return "PowerPC";
        case EM_PPC64: return "PowerPC 64-bit";
        case EM_S390: return "IBM S390";
        case EM_SPU: return "IBM SPU/SPC";
        case EM_V800: return "NEC V800 series";
        case EM_FR20: return "Fujitsu FR20";
        case EM_RH32: return "TRW RH-32";
        case EM_RCE: return "Motorola RCE";
        case EM_ARM: return "ARM";
        case EM_FAKE_ALPHA: return "Digital Alpha";
        case EM_SH: return "Hitachi SH";
        case EM_SPARCV9: return "SPARC v9 64-bit";
        case EM_TRICORE: return "Siemens Tricore";
        case EM_ARC: return "Argonaut RISC Core";
        case EM_H8_300: return "Hitachi H8/300";
        case EM_H8_300H: return "Hitachi H8/300H";
        case EM_H8S: return "Hitachi H8S";
        case EM_H8_500: return "Hitachi H8/500";
        case EM_IA_64: return "Intel Merced";
        case EM_MIPS_X: return "Stanford MIPS-X";
        case EM_COLDFIRE: return "Motorola Coldfire";
        case EM_68HC12: return "Motorola M68HC12";
        case EM_MMA: return "Fujitsu MMA Multimedia Accelerator";
        case EM_PCP: return "Siemens PCP";
        case EM_NCPU: return "Sony nCPU embeeded RISC";
        case EM_NDR1: return "Denso NDR1 microprocessor";
        case EM_STARCORE: return "Motorola Start*Core processor";
        case EM_ME16: return "Toyota ME16 processor";
        case EM_ST100: return "STMicroelectronic ST100 processor";
        case EM_TINYJ: return "Advanced Logic Corp. Tinyj emb.fam";
        case EM_X86_64: return "AMD x86-64 architecture";
        case EM_PDSP: return "Sony DSP Processor";
        case EM_PDP10: return "Digital PDP-10";
        case EM_PDP11: return "Digital PDP-11";
        case EM_FX66: return "Siemens FX66 microcontroller";
        case EM_ST9PLUS: return "STMicroelectronics ST9+ 8/16 mc";
        case EM_ST7: return "STmicroelectronics ST7 8 bit mc";
        case EM_68HC16: return "Motorola MC68HC16 microcontroller";
        case EM_68HC11: return "Motorola MC68HC11 microcontroller";
        case EM_68HC08: return "Motorola MC68HC08 microcontroller";
        case EM_68HC05: return "Motorola MC68HC05 microcontroller";
        case EM_SVX: return "Silicon Graphics SVx";
        case EM_ST19: return "STMicroelectronics ST19 8 bit mc";
        case EM_VAX: return "Digital VAX";
        case EM_CRIS: return "Axis Communications 32-bit emb.proc";
        case EM_JAVELIN: return "Infineon Technologies 32-bit emb.proc";
        case EM_FIREPATH: return "Element 14 64-bit DSP Processor";
        case EM_ZSP: return "LSI Logic 16-bit DSP Processor";
        case EM_MMIX: return "Donald Knuth's educational 64-bit proc";
        case EM_HUANY: return "Harvard University machine-independent object files";
        case EM_PRISM: return "SiTera Prism";
        case EM_AVR: return "Atmel AVR 8-bit microcontroller";
        case EM_FR30: return "Fujitsu FR30";
        case EM_D10V: return "Mitsubishi D10V";
        case EM_D30V: return "Mitsubishi D30V";
        case EM_V850: return "NEC v850";
        case EM_M32R: return "Mitsubishi M32R";
        case EM_MN10300: return "Matsushita MN10300";
        case EM_MN10200: return "Matsushita MN10200";
        case EM_PJ: return "picoJava";
        case EM_OPENRISC: return "OpenRISC 32-bit embedded processor";
        case EM_ARC_COMPACT: return "ARC International ARCompact";
        case EM_XTENSA: return "Tensilica Xtensa Architecture";
        case EM_VIDEOCORE: return "Alphamosaic VideoCore";
        case EM_TMM_GPP: return "Thompson Multimedia General Purpose Proc";
        case EM_NS32K: return "National Semi. 32000";
        case EM_TPC: return "Tenor Network TPC";
        case EM_SNP1K: return "Trebia SNP 1000";
        case EM_ST200: return "STMicroelectronics ST200";
        case EM_IP2K: return "Ubicom IP2xxx";
        case EM_MAX: return "MAX processor";
        case EM_CR: return "National Semi. CompactRISC";
        case EM_F2MC16: return "Fujitsu F2MC16";
        case EM_MSP430: return "Texas Instruments msp430";
        case EM_BLACKFIN: return "Analog Devices Blackfin DSP";
        case EM_SE_C33: return "Seiko Epson S1C33 family";
        case EM_SEP: return "Sharp embedded microprocessor";
        case EM_ARCA: return "Arca RISC";
        case EM_UNICORE: return "PKU-Unity & MPRC Peking Uni. mc series";
        case EM_EXCESS: return "eXcess configurable cpu";
        case EM_DXP: return "Icera Semi. Deep Execution Processor";
        case EM_ALTERA_NIOS2: return "Altera Nios II";
        case EM_CRX: return "National Semi. CompactRISC CRX";
        case EM_XGATE: return "Motorola XGATE";
        case EM_C166: return "Infineon C16x/XC16x";
        case EM_M16C: return "Renesas M16C";
        case EM_DSPIC30F: return "Microchip Technology dsPIC30F";
        case EM_CE: return "Freescale Communication Engine RISC";
        case EM_M32C: return "Renesas M32C";
        case EM_TSK3000: return "Altium TSK3000";
        case EM_RS08: return "Freescale RS08";
        case EM_SHARC: return "Analog Devices SHARC family";
        case EM_ECOG2: return "Cyan Technology eCOG2";
        case EM_SCORE7: return "Sunplus S+core7 RISC";
        case EM_DSP24: return "New Japan Radio (NJR) 24-bit DSP";
        case EM_VIDEOCORE3: return "Broadcom VideoCore III";
        case EM_LATTICEMICO32: return "RISC for Lattice FPGA";
        case EM_SE_C17: return "Seiko Epson C17";
        case EM_TI_C6000: return "Texas Instruments TMS320C6000 DSP";
        case EM_TI_C2000: return "Texas Instruments TMS320C2000 DSP";
        case EM_TI_C5500: return "Texas Instruments TMS320C55x DSP";
        case EM_TI_ARP32: return "Texas Instruments App. Specific RISC";
        case EM_TI_PRU: return "Texas Instruments Prog. Realtime Unit";
        case EM_MMDSP_PLUS: return "STMicroelectronics 64bit VLIW DSP";
        case EM_CYPRESS_M8C: return "Cypress M8C";
        case EM_R32C: return "Renesas R32C";
        case EM_TRIMEDIA: return "NXP Semi. TriMedia";
        case EM_QDSP6: return "QUALCOMM DSP6";
        case EM_8051: return "Intel 8051 and variants";
        case EM_STXP7X: return "STMicroelectronics STxP7x";
        case EM_NDS32: return "Andes Tech. compact code emb. RISC";
        case EM_ECOG1X: return "Cyan Technology eCOG1X";
        case EM_MAXQ30: return "Dallas Semi. MAXQ30 mc";
        case EM_XIMO16: return "New Japan Radio (NJR) 16-bit DSP";
        case EM_MANIK: return "M2000 Reconfigurable RISC";
        case EM_CRAYNV2: return "Cray NV2 vector architecture";
        case EM_RX: return "Renesas RX";
        case EM_METAG: return "Imagination Tech. META";
        case EM_MCST_ELBRUS: return "MCST Elbrus";
        case EM_ECOG16: return "Cyan Technology eCOG16";
        case EM_CR16: return "National Semi. CompactRISC CR16";
        case EM_ETPU: return "Freescale Extended Time Processing Unit";
        case EM_SLE9X: return "Infineon Tech. SLE9X";
        case EM_L10M: return "Intel L10M";
        case EM_K10M: return "Intel K10M";
        case EM_AARCH64: return "ARM AARCH64";
        case EM_AVR32: return "Amtel 32-bit microprocessor";
        case EM_STM8: return "STMicroelectronics STM8";
        case EM_TILE64: return "Tileta TILE64";
        case EM_TILEPRO: return "Tilera TILEPro";
        case EM_MICROBLAZE: return "Xilinx MicroBlaze";
        case EM_CUDA: return "NVIDIA CUDA";
        case EM_TILEGX: return "Tilera TILE-Gx";
        case EM_CLOUDSHIELD: return "CloudShield";
        case EM_COREA_1ST: return "KIPO-KAIST Core-A 1st gen.";
        case EM_COREA_2ND: return "KIPO-KAIST Core-A 2nd gen.";
        case EM_ARC_COMPACT2: return "Synopsys ARCompact V2";
        case EM_OPEN8: return "Open8 RISC";
        case EM_RL78: return "Renesas RL78";
        case EM_VIDEOCORE5: return "Broadcom VideoCore V";
        case EM_78KOR: return "Renesas 78KOR";
        case EM_56800EX: return "Freescale 56800EX DSC";
        case EM_BA1: return "Beyond BA1";
        case EM_BA2: return "Beyond BA2";
        case EM_XCORE: return "XMOS xCORE";
        case EM_MCHP_PIC: return "Microchip 8-bit PIC(r)";
        case EM_KM32: return "KM211 KM32";
        case EM_KMX32: return "KM211 KMX32";
        case EM_EMX16: return "KM211 KMX16";
        case EM_EMX8: return "KM211 KMX8";
        case EM_KVARC: return "KM211 KVARC";
        case EM_CDP: return "Paneve CDP";
        case EM_COGE: return "Cognitive Smart Memory Processor";
        case EM_COOL: return "Bluechip CoolEngine";
        case EM_NORC: return "Nanoradio Optimized RISC";
        case EM_CSR_KALIMBA: return "CSR Kalimba";
        case EM_Z80: return "Zilog Z80";
        case EM_VISIUM: return "Controls and Data Services VISIUMcore";
        case EM_FT32: return "FTDI Chip FT32";
        case EM_MOXIE: return "Moxie processor";
        case EM_AMDGPU: return "AMD GPU";
        case EM_RISCV: return "RISC-V";
        case EM_BPF: return "Linux BPF -- in-kernel virtual machine";
        case EM_CSKY: return "C-SKY";
        default: return "<<ERROR>> INVALID machine type ID <</ERROR>>";
    };
}

const char* get_et_str(unsigned int id) {
    switch (id) {
        case ET_NONE: return "No file type";
        case ET_REL: return "Relocatable file";
        case ET_EXEC: return "Executable file";
        case ET_DYN: return "Shared object file";
        case ET_CORE: return "Core file";
        default: return "<<ERROR>> INVALID file type ID <</ERROR>>";
    };
}

int decode(FILE* elf) {
    uint8_t buff[1000];
    size_t read= fread(buff, sizeof (u_int8_t), sizeof (buff) - 1, elf);
    buff[read]= '\0';

    if (!verify_special(buff)) {
        perror("Unable to verify ELF special sequence at start of file");
        return -1;
    }

    puts("Special sequence verified");

    uint8_t class_id= buff[EI_CLASS];
    const char* class= EI_CLASS_STR[class_id % ELFCLASSNUM];

    printf("ELF is of class %s\n", class);

    uint8_t data_id= buff[EI_DATA];
    const char* data= EI_DATA_STR[data_id % ELFDATANUM];

    printf("ELF is of data encoding %s\n", data);

    uint8_t version= buff[EI_VERSION];
    printf("ELF is of version %u\n", version);

    uint8_t os_abi_id= buff[EI_OSABI];
    const char* os_abi= get_osabi_str(os_abi_id);
    uint8_t os_abi_version= buff[EI_ABIVERSION];

    printf("ELF is of OS ABI type %s version %u\n", os_abi, os_abi_version);

    Elf64_Ehdr header= *(Elf64_Ehdr*)buff;
    printf(
        "ELF header information:\n"
        "  e_type: %s\n"
        "  e_machine: %s\n"
        "  e_version: %u\n"
        "  Entry point: %p\n",
        get_et_str(header.e_type),
        get_em_str(header.e_machine),
        header.e_version,
        (void*)header.e_entry
    );

    fseek(elf, (long)header.e_shoff, SEEK_SET);

    size_t sh_bytes= header.e_shentsize * header.e_shnum;
    Elf64_Shdr* sections= malloc(sh_bytes);

    fread(sections, header.e_shentsize, header.e_shnum, elf);

    Elf64_Shdr h_strs= sections[header.e_shstrndx];

    printf("hstrs at %p\n", (void*)h_strs.sh_offset);

    char* sstring_table= malloc(h_strs.sh_size);
    fseek(elf, (long)h_strs.sh_offset, SEEK_SET);
    fread(sstring_table, sizeof (uint8_t), h_strs.sh_size, elf);

    for (int i = 0; i < header.e_shnum; ++i) {
        Elf64_Shdr section= sections[i];

        printf(
                "Section %s with type %u at %ld\n",
                &sstring_table[section.sh_name],
                section.sh_type,
                section.sh_offset
        );

        if (section.sh_type == SHT_PROGBITS) {
            char* data= malloc(section.sh_size);
            fseek(elf, section.sh_offset, SEEK_SET);
            fread(data, sizeof (uint8_t), section.sh_size, elf);

            for (int j = 0; j < section.sh_size; ++j) {
                printf("%02X ", (uint8_t)data[j]);
                if (j % 16 == 15) putchar('\n');
            }
            putchar('\n');
            free(data);
        }
    }

    free(sections);

    return 0;
}