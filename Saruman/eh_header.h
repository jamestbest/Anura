//
// Created by james on 09/01/26.
//

#ifndef EH_HEADER_H
#define EH_HEADER_H
#include <stdbool.h>

// types and stuff for the Exception Handling API described in LSB-Core-generic

typedef enum PEValueFormat {
    DW_EH_PE_absptr= 0x00,      // literal pointer whose size is arch specific
    DW_EH_PE_uleb128= 0x01,     // ULEB128
    DW_EH_PE_udata2= 0x02,      // 2 byte unsigned
    DW_EH_PE_udata4= 0x03,      // 4 byte unsigned
    DW_EH_PE_udata8= 0x04,      // 8 byte unsigned
    DW_EH_PE_sleb128= 0x09,     // LEB128
    DW_EH_PE_sdata2= 0x0A,      // 2 byte signed
    DW_EH_PE_sdata4= 0x0B,      // 4 byte signed
    DW_EH_PE_sdata8= 0x0C,      // 8 byte signed
} PEValueFormat;

typedef enum PEApplication {
    DW_EH_PE_pcrel= 0x10,       // Value is relative to the Program Counter
    DW_EH_PE_textrel= 0x20,     // Value is relative to the start of the .text section
    DW_EH_PE_datarel= 0x30,     // Value is relative to the start of the .got or .eh_frame_hdr section
    DW_EH_PE_funcrel= 0x40,     // Value is relative to the start of the function
    DW_EH_PE_aligned= 0x50,     // Value is aligned to an address unit sized boundary
} PEApplication;

typedef enum PESpecial {
    DW_EH_PE_omit= 0xff,        // NO value is present
} PESpecial;

typedef struct PointerEncoding {
    PEValueFormat value;
    PEApplication application;
} PointerEncoding;

typedef struct Pointer {
    PEApplication application;
    bool signed_ptr;
    union {
        uint64_t ptr_u;
        int64_t  ptr_s;
    };
} Pointer;

#endif //EH_HEADER_H
