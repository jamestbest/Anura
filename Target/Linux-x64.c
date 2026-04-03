//
// Created by james on 30/10/25.
//

#include "Linux-x64.h"

#include <elf.h>

#include "../main.h"
#include "../TargetOS/Linux.h"

#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/uio.h>

#include "Errors.h"
#include "Palantir/Palantir.h"
#include "Saruman/Saruman.h"

#define SW_INT_TYPE (unsigned char)
#define SW_INT_CODE 0xCC
_Static_assert(SW_INT_TYPE SW_INT_CODE == SW_INT_CODE);

_Static_assert(sizeof SW_INT_TYPE == sizeof ((BPInfo){0}.data.shadow), "The shadow element should encapsulate all data lost from the Software interrupt code");

long long aligned_write(uintptr_t address, uint8_t value, uint8_t* existing_value);

long long remove_hw_bp(uint8_t bp_register) {
    long long r7= ptrace(PTRACE_PEEKUSER, target.pid, offsetof(struct user, u_debugreg[7]));

    r7 &= 0 << (bp_register << 1); // disable LOCAL BREAKPOINT

    long long res= 0;
    res= ptrace(PTRACE_POKEUSER, target.pid, offsetof(struct user, u_debugreg[7]), r7);
    if (res != 0) return res;

    res= ptrace(PTRACE_POKEUSER, target.pid, offsetof(struct user, u_debugreg[bp_register]), (long long)0);
    printf("Removed hw bp in register %u\n", bp_register);
    return res;
}

long long remove_sw_bp(BPInfo* info) {
    const uintptr_t address= info->addr;
    const long long res= aligned_write(address, info->data.shadow, NULL);
    if (res != 0) return res;

    printf("Removed sw bp @%#lx with shadow %u\n", info->addr, info->data.shadow);

    return SUCCESS;
}

long long remove_bp(BPInfo* bp) {
    long long res;
    switch (bp->type) {
        case BP_HARDWARE: res= remove_hw_bp(bp->data.bp); break;
        case BP_SOFTWARE: res= remove_sw_bp(bp); break;

        case BP_SOURCE_SINGLE_STEP_TRAP:
        case BP_TYPE_COUNT:
        default:
            assert(false);
    }

    return res;
}


long long remove_bp_at_addr_cfa(uintptr_t addr, BP_REASON reason, uintptr_t cfa) {
    const size_t i= BPAddressInfo_arr_search_i(&bp_info, addr);
    BPAddressInfo* info= BPAddressInfo_arr_ptr(&bp_info, i);
    if (!info) return FAIL;

    decrement_by_reason(info, reason);

    if (cfa != -1) {
        size_t idx=-1;
        for (int j = 0; j < info->bps.pos; ++j) {
            BP* bp= BP_arr_ptr(&info->bps, j);
            if (bp->cfa == cfa) {
                idx= j;
            }
        }
        BP_arr_remove(&info->bps, idx);
    }

    if (info->bp_count == 0) {
        const long long res= remove_bp(&info->canonical_bp);
        if (res == SUCCESS) {
            BPAddressInfo_arr_remove(&bp_info, i);
        }
    }

    update_breakpoint_displays(NULL);

    return SUCCESS;
}

long long remove_bp_at_addr(uintptr_t addr, BP_REASON reason) {
    return remove_bp_at_addr_cfa(addr, reason, -1);
}


long long aligned_write(uintptr_t address, uint8_t value, uint8_t* existing_value) {
    // we want to read just 1 byte of data for the shadow, but this might not be an aligned read
    // so we'll find the nearest 8 aligned boundry which includes the address and then shift out the rest
    //  save this alignment offset for later writing the shadow back
    const uintptr_t a_addr= ((long long)address & ~0b111);
    unsigned int offset= address - a_addr;

    errno= 0;
    uint64_t data= ptrace(PTRACE_PEEKDATA, target.pid, a_addr, a_addr);

    if (data == -1 && errno) {
        return errno;
    }

    offset <<= 3; // * 8 to get the number of bits
    if (existing_value)
        *existing_value= ((data >> offset) & 0xFFUL);
    data= ((long long)value << offset) | (data & ~(0xFFL << offset));

    return ptrace(PTRACE_POKETEXT, target.pid, a_addr, data);
}

long long readd_sw_bp(BPInfo* bp) {
    printf("Readding sw bp @%#lx\n", bp->addr);
    if (bp->type != BP_SOFTWARE) return FAIL;

    target.target_get_data_runtime(bp->addr, 16);
    long long res= aligned_write(bp->addr, SW_INT_CODE, &bp->data.shadow);
    target.target_get_data_runtime(bp->addr, 16);

    target.target_get_data_runtime(target.target_get_pc(), 16);

    return res;
}

long long place_bp_(uintptr_t address, uint32_t line, BP_REASON reason, uintptr_t cfa, void (*callback)(void* data), void* data) {
    long long r7= ptrace(PTRACE_PEEKUSER, target.pid, offsetof(struct user, u_debugreg[7]));

    bool bp_used[4]= {0};
    unsigned int free_bp= -1;
    for (int j = 0; j < 4; ++j) {
        bool used= ((r7 >> (j << 1)) & 0b11);
        bp_used[j]= used;
        if (!used && (free_bp == (unsigned int)-1)) free_bp= j;
    }

    BPAddressInfo* addr_info;
    // if there is a local or global breakpoint enabled for all bps then we cannot create a hardware breakpoint
    if (free_bp == -1) {
        uint8_t shadow;

        target.target_get_data_runtime(address - 8, 16);

        const long long res= aligned_write(address, SW_INT_CODE, &shadow);
        if (res != 0) return res;

        target.target_get_data_runtime(address - 8, 16);

        addr_info= get_or_add_bp_address_info(address,
            (BPInfo){
                .addr= address,
                .line= line,
                .type= BP_SOFTWARE,
                .data.shadow= shadow
            },
            reason
        );

        g_idle_add(update_breakpoint_memory, NULL);

        goto place_bp_end;
    }

    r7 |= 1 << (free_bp << 1); // enable LOCAL BREAKPOINT free_bp
    r7 &= (~(0b11 << (16 + (free_bp << 2)))); // set R/Wx to 00 I.e. BRK INST
    r7 &= (~(0b11 << (18 + (free_bp << 2)))); // set LENx to 00 FOLLOWING R/W0 being 00 (Vol. 3B 19-5)

    long long res= 0;
    res= ptrace(PTRACE_POKEUSER, target.pid, offsetof(struct user, u_debugreg[7]), r7);
    if (res != 0) return res;

    res= ptrace(PTRACE_POKEUSER, target.pid, offsetof(struct user, u_debugreg[free_bp]), (long long)address);
    if (res != 0) return res;

    addr_info= get_or_add_bp_address_info(address,
        (BPInfo) {
            .addr= address,
            .line= line,
            .type= BP_HARDWARE,
            .data.bp= free_bp
        },
        reason
    );

place_bp_end:
    if (callback != NULL) {
        BP_arr_add(&addr_info->bps, (BP) {
            .reason= reason,
            .cfa= cfa,
            .callback= callback,
            .data= data
        });
    }

    update_breakpoint_displays(NULL);

    return SUCCESS;
}

long long place_bp(uintptr_t address, uint32_t line, BP_REASON reason);
long long place_temp_bp(uintptr_t address, BP_REASON reason) {
    return place_bp(address, -1, reason);
}

long long place_bp(uintptr_t address, uint32_t line, BP_REASON reason) {
    return place_bp_(address, line, reason, -1, NULL, NULL);
}

long long place_bp_with_cfa(uintptr_t addr, uintptr_t cfa, BP_REASON reason, void (*callback)(void* data), void* data) {
    return place_bp_(addr, -1, reason, cfa, callback, data);
}

void breakpoint_hit_cleanup() {
    // clear R6 for the next
    long long r6= 1 << 16 | 1 << 11; // Enable RTM & BLD (19-4 Vol. 3B)
    ptrace(PTRACE_POKEUSER, target.pid, offsetof(struct user, u_debugreg[6]), r6);
}

void interrupt() {
    kill(target.pid, SIGINT);
}

void interrupt_handler(int sig) {
    signal(sig, SIG_IGN); // ignore the signal :)

    // we'll send this to the other process
    target.target_interrupt();
}

void interrupt_handler_setup() {
    signal(SIGINT, interrupt_handler);
}

GeneralRegs get_regs(bool* succ) {
    GeneralRegs regs;

    errno= 0;
    const long res= ptrace(PTRACE_GETREGS, target.pid, NULL, &regs);
    if (res == -1 && errno != 0) {
        *succ=false;
        return regs;
    }

    *succ= true;
    return regs;
}

typedef enum CPUID_QUERY {
    CPUID_QUERY_XSAVE_INFO= 0x0D
} CPUID_QUERY;

typedef enum CPUID_QUERY_LEAF {
    CPUID_QUERY_XSAVE_LEAF_BASE=0,
    CPUID_QUERY_XSAVE_LEAF_EXT=1,
} CPUID_QUERY_LEAF;

typedef struct CPUID_XSaveInfoRes {
    union {
        struct {
            uint32_t x87_state: 1;
            uint32_t sse_state: 1;
            uint32_t avx_state: 1;
            uint32_t mpx_state: 2;
            uint32_t avx512_state: 3;
            uint32_t pkru_state: 1;
        };
        uint32_t eax;
    };
    union {
        uint32_t max_sz_xcr0;
        uint32_t ebx;
    };
    union {
        uint32_t max_sz_xsave;
        uint32_t ecx;
    };
    uint32_t edx;
} CPUID_XSaveInfoRes;

typedef struct CPUID_XSaveExtendedInfoRes {
    union {
        struct {
            uint32_t xsaveopt: 1;
            uint32_t xsavec: 1;
            uint32_t xgetbv_ecx1: 1;
            uint32_t xss: 1;
            uint32_t xfd: 1;
        };
        uint32_t eax;
    };
    union {
        uint32_t max_xsave_size;
        uint32_t ebx;
    };
    union {
        struct {
            uint32_t edx;
            uint32_t ecx;
        };
        __uint128_t ia32_xss_bitmap;
    };
} CPUID_XSaveExtendedInfoRes;

typedef struct CPUID_XSaveStateInfoRes {
    uint32_t size;
    uint32_t offset;
    union {
        struct {
            uint32_t user_supervisor_state: 1;
            uint32_t alignment_state: 1;
        };
        uint32_t ecx;
    };
} CPUID_XSaveStateInfoRes;

typedef struct CPUID_BASE_INFO {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} CPUID_BASE_INFO;

typedef union CPUID_RESULT {
    CPUID_BASE_INFO base;
    CPUID_XSaveInfoRes xsave_info;
    CPUID_XSaveExtendedInfoRes xsave_ext_info;
    CPUID_XSaveStateInfoRes xsave_state_info;
} CPUID_RESULT;

#include <cpuid.h>

CPUID_RESULT cpuid_query(CPUID_QUERY query, CPUID_QUERY_LEAF leaf) {
    uint32_t eax, ebx, ecx, edx;
    __cpuid_count(query, leaf, eax, ebx, ecx, edx);

    switch (query) {
        case CPUID_QUERY_XSAVE_INFO: {
            if (leaf == CPUID_QUERY_XSAVE_LEAF_BASE) return (CPUID_RESULT) {.xsave_info= (CPUID_XSaveInfoRes) {
                .eax= eax,
                .max_sz_xcr0= ebx,
                .max_sz_xsave= ecx,
                .edx= edx,
            }};
            if (leaf == CPUID_QUERY_XSAVE_LEAF_EXT) return (CPUID_RESULT) {.xsave_ext_info= (CPUID_XSaveExtendedInfoRes) {
                .eax= eax,
                .ebx= ebx,
                .ecx= ecx,
                .edx= edx,
            }};
            return (CPUID_RESULT) {
                .xsave_state_info= {
                    .size= eax,
                    .offset= ebx,
                    .ecx= ecx,
                }
            };
        }
    }

    return (CPUID_RESULT) {.base= (CPUID_BASE_INFO) {
        .eax = eax,
        .ebx = ebx,
        .ecx = ecx,
        .edx = edx
    }};
}

#include <xmmintrin.h>

typedef struct MMRegEncoding {
    uint8_t val[10];
    uint8_t reserved[6];
} MMRegEncoding;

typedef struct XSaveLegacyData {
    uint16_t fcw;
    uint16_t fsw;
    uint8_t ftw;
    uint8_t reserved_00;
    uint16_t fop;
    uint32_t fip_31_00;
    uint16_t fip_47_32_or_fcs;
    uint16_t fip_63_48_or_reserved;
    uint32_t fdp;
    uint16_t fds_or_fdp_47_32;
    uint16_t fdp_63_48_or_reserved;
    uint32_t mxcsr;
    uint32_t mxcsr_mask;
    MMRegEncoding mm_regs[8];
    __m128 xmm_regs[16];
} XSaveLegacyData;

typedef struct XSaveBV {
    uint64_t x87_state: 1;
    uint64_t sse_state: 1;
    uint64_t avx_state: 1;
    uint64_t mpx_bnd_reg_state: 1;
    uint64_t mpx_bnd_csr_state: 1;
    uint64_t avx512_opmask_state: 1;
    uint64_t avx512_zmm_hi256_state: 1;
    uint64_t avx512_hi16_zmm_state: 1;
    uint64_t pt_state: 1;
    uint64_t pkru_state: 1;
    uint64_t pasid_state: 1;
    uint64_t cet_u_state: 1;
    uint64_t cet_s_state: 1;
    uint64_t hdc_state: 1;
    uint64_t uintr_state: 1;
    uint64_t lbr_state: 1;
    uint64_t hwp_state: 1;
    uint64_t amx_tilecfg_state: 1;
    uint64_t amx_tiledata_state: 1;
} XSaveBV;

typedef struct XCompBV {
    uint64_t format: 63;
    uint64_t is_ext_format: 1;
} XCompBV;

typedef struct XSaveHeader {
    XSaveBV xsave_bv;
    XCompBV xcomp_bv;
} XSaveHeader;

typedef struct XSaveRegisters {
    uint8_t has_x87: 1;
    uint8_t has_sse: 1;
    uint8_t has_avx: 1;
    uint8_t has_avx512_opmasks: 1;
    uint8_t has_avx512_zmmhis : 1;
    uint8_t has_avx512_hi16_zmms: 1;

    MMRegEncoding mm_regs[8];
    __m128 xmm_regs[16];
    __m128 ymm_hi_regs[16];
    struct {
        uint8_t val[32];
    } zmm_hi_regs[16];
    struct {
        __m128 val[4];
    } zmm_hi16_regs[16];

    uint64_t k_regs[8];
} XSaveRegisters;

// there are currently 19 defined state components Sec. 13.1 XSAVE-SUPPORTED FEATURES AND STATE-COMPONENT BITMAPS
#define MAX_DEFINED_STATE_COMP 19
CPUID_XSaveStateInfoRes state_comp_info[MAX_DEFINED_STATE_COMP];
bool state_comp_info_loaded= false;

void load_state_component_info() {
    if (state_comp_info_loaded) return;

    // state components 0 & 1 are the x87 and sse which have fixed size information in the legacy section
    for (int i = 2; i < MAX_DEFINED_STATE_COMP; ++i) {
        state_comp_info[i]= cpuid_query(CPUID_QUERY_XSAVE_INFO, i).xsave_state_info;
    }

    state_comp_info_loaded= true;
}

typedef enum STATE_COMPONENT {
    SC_X87=0,
    SC_SSE=1,
    SC_AVX=2,
    SC_BNDREGS=3,
    SC_BNDCSR=4,
    SC_AVX512_OPMASK=5,
    SC_AVX512_HI256=6,
    SC_AVX512_HI16=7,
} STATE_COMPONENT;

#define BIT_AT(data, idx) ((data >> idx) & 1)

void decode_ext_format(const XSaveHeader* header) {
    uint8_t last_present_idx= 0;
    for (int i = 2; i < MAX_DEFINED_STATE_COMP; ++i) {
        if (BIT_AT(header->xcomp_bv.format, i) == 0) continue;

        if (last_present_idx == 0) {
            state_comp_info[i].offset= 576;
            last_present_idx= i;
        } else {
            const uint32_t last_offset= state_comp_info[last_present_idx].offset;
            const uint32_t last_size= state_comp_info[last_present_idx].size;

            const bool align= state_comp_info[i].alignment_state;

            if (align) {
                state_comp_info[i].offset= (last_offset + last_size + 63) & ~63; // little bit manipulation curtesy of Prof. Bagley
            } else {
                state_comp_info[i].offset= last_offset + last_size;
            }
        }
    }
}

void set_xsave_data(const uint8_t* data, XSaveRegisters* regs) {
    for (STATE_COMPONENT i = 2; i < MAX_DEFINED_STATE_COMP; ++i) {
        switch (i) {
            case SC_X87:
            case SC_SSE:
                break;

            case SC_AVX: {
                // Bytes 127:0 of the AVX-state section are used for YMM0_H–YMM7_H. Bytes 255:128 are used for YMM8_H–YMM15_H
                memcpy(&regs->ymm_hi_regs, data + state_comp_info[i].offset, state_comp_info[i].size);
                break;
            }

            case SC_BNDREGS:
            case SC_BNDCSR:
                break;

            case SC_AVX512_OPMASK: {
                memcpy(&regs->k_regs, data + state_comp_info[i].offset, state_comp_info[i].size);
                break;
            }
            case SC_AVX512_HI256: {
                memcpy(&regs->zmm_hi_regs, data + state_comp_info[i].offset, state_comp_info[i].size);
                break;
            }
            case SC_AVX512_HI16: {
                memcpy(&regs->zmm_hi16_regs, data + state_comp_info[i].offset, state_comp_info[i].size);
                break;
            }
            default: break;
        }
    }
}

XSaveRegisters get_all_regs(bool* succ) {
    // the size of this is slightly dynamic based on the processor features
    //  an assumption could be avx2, but this should support everything
    const CPUID_XSaveExtendedInfoRes info= cpuid_query(CPUID_QUERY_XSAVE_INFO, CPUID_QUERY_XSAVE_LEAF_EXT).xsave_ext_info;

    struct iovec iov;
    iov.iov_len= info.max_xsave_size;
    iov.iov_base= malloc(iov.iov_len);

    long long res= ptrace(PTRACE_GETREGSET, target.pid, (void*)NT_X86_XSTATE, &iov);
    // this is in a x64 specific format, and most likely the 'compact' format so need to decode
    //  based on Sec. 13.4.3 (intel combined)

    /*      LEGACY REGION (512 bytes)
     *      HEADER (64 bytes)
     *      EXTENDED REGION (x bytes (based on max_xsave_size))
     */
    const XSaveLegacyData data= *(XSaveLegacyData*)iov.iov_base;
    const XSaveHeader header= *(XSaveHeader*)(iov.iov_base + 512);
    const uint8_t* extended_data= (uint8_t*)iov.iov_base + 512 + 64;

    XSaveRegisters regs;
    regs.has_x87= header.xsave_bv.x87_state;
    regs.has_sse= header.xsave_bv.sse_state;
    regs.has_avx= header.xsave_bv.avx_state;
    regs.has_avx512_opmasks= header.xsave_bv.avx512_opmask_state;
    regs.has_avx512_zmmhis= header.xsave_bv.avx512_zmm_hi256_state;
    regs.has_avx512_hi16_zmms= header.xsave_bv.avx512_hi16_zmm_state;

    if (regs.has_x87) memcpy(&regs.mm_regs, &data.mm_regs, sizeof(regs.mm_regs));
    if (regs.has_sse) memcpy(&regs.xmm_regs, &data.xmm_regs, sizeof(regs.xmm_regs));

    // this describes the layout of the extended region
    load_state_component_info();

    if (header.xcomp_bv.is_ext_format) {
        decode_ext_format(&header);
    }

    set_xsave_data(extended_data, &regs);

    return regs;
}

uintptr_t get_pc() {
    bool succ;
    const GeneralRegs regs= get_regs(&succ);

    if (succ) return regs.rip;
    return -1;
}

long long cf_main(bool is_continue) {
    // each time cf moves forward it could be that we're sitting on a sw bp
    // in which case we need to replace the shadow then single step
    // then replace the shadow again and either continue or stop based on the cf movement
    printf("Checking sw bp\n");
    const uintptr_t pc = target.target_get_pc();
    const BPAddressInfo* bp= BPAddressInfo_arr_search_ie(&bp_info, pc);

    if (!bp || bp->canonical_bp.type != BP_SOFTWARE) {
        printf("There is no sw bp here\n");
        if (is_continue) return target.target_unsafe_continue();
        return target.target_unsafe_single_step();
    }

    printf("There is a sw bp here\n");

    const uint8_t shadow= bp->canonical_bp.data.shadow;
    const uintptr_t addr= pc;

    target.target_get_data_runtime(addr - 8, 16);

    const long long res= aligned_write(addr, shadow, NULL);
    if (res != SUCCESS) return res;

    target.target_get_data_runtime(addr - 8, 16);

    target.sw_bp_to_readd_addr= bp->canonical_bp.addr;
    target.sw_bp_should_continue= is_continue;

    target.target_unsafe_single_step();

    return SUCCESS;
}

long long cf_continue() {
    return target.target_cf_main(true);
}

long long unsafe_continue() {
    return ptrace(PTRACE_CONT, target.pid, 0, 0);
}

#define DW_REG_ENC_RAX 0
#define DW_REG_ENC_RDX 1
#define DW_REG_ENC_RCX 2
#define DW_REG_ENC_RBX 3
#define DW_REG_ENC_RSI 4
#define DW_REG_ENC_RDI 5
#define DW_REG_ENC_RBP 6
#define DW_REG_ENC_RSP 7
#define DW_REG_ENC_R8 8
#define DW_REG_ENC_R9 9
#define DW_REG_ENC_R10 10
#define DW_REG_ENC_R11 11
#define DW_REG_ENC_R12 12
#define DW_REG_ENC_R13 13
#define DW_REG_ENC_R14 14
#define DW_REG_ENC_R15 15
#define DW_REG_ENC_RIP 16

uint64_t get_general_reg_value_using(const uint16_t register_id, GeneralRegs* regs, bool* succ) {
    *succ= true;

    switch (register_id) {
        case DW_REG_ENC_RAX: return regs->rax;
        case DW_REG_ENC_RDX: return regs->rdx;
        case DW_REG_ENC_RCX: return regs->rcx;
        case DW_REG_ENC_RBX: return regs->rbx;
        case DW_REG_ENC_RSI: return regs->rsi;
        case DW_REG_ENC_RDI: return regs->rdi;
        case DW_REG_ENC_RBP: return regs->rbp;
        case DW_REG_ENC_RSP: return regs->rsp;
        case DW_REG_ENC_R8: return regs->r8;
        case DW_REG_ENC_R9: return regs->r9;
        case DW_REG_ENC_R10: return regs->r10;
        case DW_REG_ENC_R11: return regs->r11;
        case DW_REG_ENC_R12: return regs->r12;
        case DW_REG_ENC_R13: return regs->r13;
        case DW_REG_ENC_R14: return regs->r14;
        case DW_REG_ENC_R15: return regs->r15;
        case DW_REG_ENC_RIP: return regs->rip;

        case 49: return regs->eflags;
        case 50: return regs->es;
        case 51: return regs->cs;
        case 52: return regs->ss;
        case 53: return regs->ds;
        case 54: return regs->fs;
        case 55: return regs->gs;

        case 58: return regs->fs_base;
        case 59: return regs->gs_base;
        default: *succ=false;
    }

    return 0x12345678;
}

bool get_flag(const FLAGS flag) {
    bool succ;
    GeneralRegs regs= get_regs(&succ);
    assert(succ);

    switch (flag) {
        case FLAG_ZERO: {
            return regs.eflags & FLAG_ZERO;
        }
        default: assert(false);
    }
}

#define EFLAG_ZF 0x0040
#define EFLAG_SF 0x0080
#define EFLAG_OF 0x0800

bool check_comparison(const COMPARISONS comparison) {
    bool succ;
    const GeneralRegs regs= get_regs(&succ);
    assert(succ);

    const bool zf= regs.eflags & EFLAG_ZF;
    const bool sf= regs.eflags & EFLAG_SF;
    const bool of= regs.eflags & EFLAG_OF;

    switch (comparison) {
        case COMPARE_EQ: {
            return zf;
        }
        case COMPARE_NEQ: {
            return !zf;
        }
        case COMPARE_GT: {
            return zf && (sf == of);
        }
        case COMPARE_GTE: {
            return sf == of;
        }
        case COMPARE_LESS: {
            return sf != of;
        }
        case COMPARE_LESSEQ: {
            return zf || sf != of;
        }
        default:
            assert(false);
    }
}

uint64_t get_general_reg_value(const uint16_t register_id, bool* succ) {
    GeneralRegs regs= get_regs(succ);
    if (!*succ) return -1;

    return get_general_reg_value_using(register_id, &regs, succ);
}

// this mapping is based on the ABI https://gitlab.com/x86-psABIs/x86-64-ABI
// for linux-x64 this mapping is dwarf numbers to actual registers
Reg get_register_value(const uint16_t register_id) {
    bool succ;
    uint64_t val= get_general_reg_value(register_id, &succ);
    if (succ) return (Reg){.type= REGVAL_GENERAL, .value= {.general= val}};

    XSaveRegisters all_regs= get_all_regs(&succ);
    if (!succ) return (Reg){.type= REGVAL_ERROR};

    Reg res= {.type= REGVAL_VECTOR, .value= {.vector= {0}}};

    // need to collect the different sections of the zmm/ymm registers IF they are present
    if (register_id >= 17 && register_id <= 32) {
        memcpy(&res.value.vector, &all_regs.xmm_regs[register_id - 17], sizeof(__m128));
        if (all_regs.has_avx)
            memcpy(&res.value.vector[1], &all_regs.ymm_hi_regs[register_id - 17], sizeof(__m128));

        if (all_regs.has_avx512_zmmhis)
            memcpy(&res.value.vector[2], &all_regs.zmm_hi_regs[register_id - 17], sizeof(__m128) << 1);

        return res;
    }

    if (register_id >= 67 && register_id <= 82) {
        if (all_regs.has_avx512_hi16_zmms)
            memcpy(&res.value.vector, &all_regs.zmm_hi16_regs[register_id - 67], sizeof(__m128) << 2);

        return res;
    }

    return (Reg) {.type= REGVAL_ERROR};
}

uint64_t get_general_reg_at(const uintptr_t addr) {
    return ptrace(PTRACE_PEEKDATA, target.pid, addr, 0);
}

#define REG_COUNT 17

const char* get_register_name(uint16_t register_id) {
    switch (register_id) {
        case DW_REG_ENC_RAX: return "RAX";
        case DW_REG_ENC_RDX: return "RDX";
        case DW_REG_ENC_RCX: return "RCX";
        case DW_REG_ENC_RBX: return "RBX";
        case DW_REG_ENC_RSI: return "RSI";
        case DW_REG_ENC_RDI: return "RDI";
        case DW_REG_ENC_RBP: return "RBP";
        case DW_REG_ENC_RSP: return "RSP";
        case DW_REG_ENC_R8: return "R8";
        case DW_REG_ENC_R9: return "R9";
        case DW_REG_ENC_R10: return "R10";
        case DW_REG_ENC_R11: return "R11";
        case DW_REG_ENC_R12: return "R12";
        case DW_REG_ENC_R13: return "R13";
        case DW_REG_ENC_R14: return "R14";
        case DW_REG_ENC_R15: return "R15";
        case DW_REG_ENC_RIP: return "RIP";
        default: return "Unknown register";
    }
}

LabelledRegs get_labelled_regs() {
    LabelledRegs lregs= (LabelledRegs) {
        .string_buff= buffer_create(BUFF_MIN),
        .regs= LabelledReg_arr_construct(REG_COUNT)
    };

    for (int i = 0; i < REG_COUNT; ++i) {
        const char* name= "Unknown register";
        switch (i) {
            case DW_REG_ENC_RAX: name= "RAX"; break;
            case DW_REG_ENC_RDX: name= "RDX"; break;
            case DW_REG_ENC_RCX: name= "RCX"; break;
            case DW_REG_ENC_RBX: name= "RBX"; break;
            case DW_REG_ENC_RSI: name= "RSI"; break;
            case DW_REG_ENC_RDI: name= "RDI"; break;
            case DW_REG_ENC_RBP: name= "RBP"; break;
            case DW_REG_ENC_RSP: name= "RSP"; break;
            case DW_REG_ENC_R8: name= "R8"; break;
            case DW_REG_ENC_R9: name= "R9"; break;
            case DW_REG_ENC_R10: name= "R10"; break;
            case DW_REG_ENC_R11: name= "R11"; break;
            case DW_REG_ENC_R12: name= "R12"; break;
            case DW_REG_ENC_R13: name= "R13"; break;
            case DW_REG_ENC_R14: name= "R14"; break;
            case DW_REG_ENC_R15: name= "R15"; break;
            case DW_REG_ENC_RIP: name= "RIP"; break;
            default: break;
        }

        const Reg reg= target.target_get_reg(i);
        LabelledReg_arr_add(&lregs.regs, (LabelledReg) {
            .name= name,
            .reg_num= i,
            .reg= reg
        });
    }

    return lregs;
};

bool set_reg_struct_value(GeneralRegs* regs, uint16_t register_id, uint64_t value) {
    switch (register_id) {
        case DW_REG_ENC_RAX: regs->rax= value; return true;
        case DW_REG_ENC_RBX: regs->rbx= value; return true;
        case DW_REG_ENC_RCX: regs->rcx= value; return true;
        case DW_REG_ENC_RDX: regs->rdx= value; return true;
        case DW_REG_ENC_R8: regs->r8= value; return true;
        case DW_REG_ENC_R9: regs->r9= value; return true;
        case DW_REG_ENC_R10: regs->r10= value; return true;
        case DW_REG_ENC_R11: regs->r11= value; return true;
        case DW_REG_ENC_R12: regs->r12= value; return true;
        case DW_REG_ENC_R13: regs->r13= value; return true;
        case DW_REG_ENC_R14: regs->r14= value; return true;
        case DW_REG_ENC_R15: regs->r15= value; return true;
        case DW_REG_ENC_RIP: regs->rip= value; return true;
        case DW_REG_ENC_RBP: regs->rbp= value; return true;
        case DW_REG_ENC_RSP: regs->rsp= value; return true;
        case DW_REG_ENC_RSI: regs->rsi= value; return true;
        case DW_REG_ENC_RDI: regs->rdi= value; return true;
        default: return false;
    }
}

int linux_x64_init_target(Target* t) {
    linux_init_target(t);

    t->target_place_bp_at_addr= place_bp;
    t->target_remove_bp_at_addr= remove_bp_at_addr;
    t->target_remove_bp_at_addr_cfa= remove_bp_at_addr_cfa;
    t->target_breakpoint_hit_cleanup= breakpoint_hit_cleanup;
    t->target_interrupt= interrupt;
    t->target_interrupt_handler_setup= interrupt_handler_setup;
    t->target_cf_continue= cf_continue;
    t->target_unsafe_continue= unsafe_continue;
    t->target_get_pc= get_pc;

    t->target_get_general_regs= get_regs;
    t->target_get_general_reg_using= get_general_reg_value_using;
    t->target_get_reg= get_register_value;
    t->target_get_general_reg_at= get_general_reg_at;
    t->target_get_labelled_regs= get_labelled_regs;
    t->target_set_reg_struct_value= set_reg_struct_value;

    t->target_place_temp_bp= place_temp_bp;
    t->target_aligned_write= aligned_write;
    t->target_readd_sw_bp= readd_sw_bp;

    t->target_cf_main= cf_main;

    t->target_place_bp_with_cfa= place_bp_with_cfa;

    return 0;
}
