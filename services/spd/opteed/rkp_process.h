#ifndef RKP_PROCESS_H
#define RKP_PROCESS_H

#include <assert.h>
#include <errno.h>
#include <stddef.h>

#include <arch_helpers.h>
#include <bl31/bl31.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <common/runtime_svc.h>
#include <lib/el3_runtime/context_mgmt.h>
#include <plat/common/platform.h>
#include <tools_share/uuid.h>

#include "opteed_private.h"
#include "teesmc_opteed.h"
#include "teesmc_opteed_macros.h"

#define NULL_PTR (long unsigned int)0lu

//定义功能对应的smc指令id
#define TEESMC_OPTEED_FUNCID_RKP_PTM_INIT 20
#define TEESMC_OPTEED_RKP_PTM_INIT \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_RKP_PTM_INIT)
#define TEESMC_OPTEED_FUNCID_RKP_PTM_GETAPT 21
#define TEESMC_OPTEED_RKP_PTM_GETAPT \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_RKP_PTM_GETAPT)
#define TEESMC_OPTEED_FUNCID_RKP_PTM_RELEASEAPT 22
#define TEESMC_OPTEED_RKP_PTM_RELEASEAPT \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_RKP_PTM_RELEASEAPT)
#define TEESMC_OPTEED_FUNCID_RKP_SET_PAGETABLE 23
#define TEESMC_OPTEED_RKP_SET_PAGETABLE \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_RKP_SET_PAGETABLE)
#define TEESMC_OPTEED_FUNCID_RKP_INSTR_SIMULATION 24
#define TEESMC_OPTEED_RKP_INSTR_SIMULATION \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_RKP_INSTR_SIMULATION)
#define TEESMC_OPTEED_FUNCID_RKP_CLEAR_PAGE 25
#define TEESMC_OPTEED_RKP_CLEAR_PAGE \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_RKP_CLEAR_PAGE)
#define TEESMC_OPTEED_FUNCID_RKP_COPY_PAG 26
#define TEESMC_OPTEED_RKP_COPY_PAGE \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_RKP_COPY_PAG)
#define TEESMC_OPTEED_FUNCID_RKP_MEM_SET 27
#define TEESMC_OPTEED_RKP_MEM_SET \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_RKP_MEM_SET)

#define TEESMC_OPTEED_FUNCID_RKP_SET_ROADDR 40
#define TEESMC_OPTEED_RKP_SET_ROADDR \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_RKP_SET_ROADDR)
#define TEESMC_OPTEED_FUNCID_RKP_SET_FORBID_FLAG 41
#define TEESMC_OPTEED_RKP_SET_FORBID_FLAG \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_RKP_SET_FORBID_FLAG)
#define TEESMC_OPTEED_FUNCID_RKP_SET_PXN 42
#define TEESMC_OPTEED_RKP_SET_PXN \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_RKP_SET_PXN)
#define TEESMC_OPTEED_FUNCID_PKM_PROTECT_KEY_CODE 51
#define TEESMC_OPTEED_PKM_PROTECT_KEY_CODE \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_PKM_PROTECT_KEY_CODE)
#define TEESMC_OPTEED_FUNCID_PKM_SELINUX 52
#define TEESMC_OPTEED_PKM_SELINUX \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_PKM_SELINUX)

#define TEESMC_OPTEED_FUNCID_KILL_HOOK 80
#define TEESMC_OPTEED_KILL_HOOK \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_KILL_HOOK)
#define TEESMC_OPTEED_FUNCID_PUSH_TASKADDR 81
#define TEESMC_OPTEED_PUSH_TASKADDR \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_PUSH_TASKADDR)
#define TEESMC_OPTEED_FUNCID_SET_PUSH_TASKADDR_FLAG 82
#define TEESMC_OPTEED_SET_PUSH_TASKADDR_FLAG \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_SET_PUSH_TASKADDR_FLAG)

#define TEESMC_OPTEED_FUNCID_SET_PID_AND_STACK 90
#define TEESMC_OPTEED_SET_PID_AND_STACK \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_SET_PID_AND_STACK)
#define TEESMC_OPTEED_FUNCID_FREE_PID_AND_STACK 91
#define TEESMC_OPTEED_FREE_PID_AND_STACK \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_FREE_PID_AND_STACK)
#define TEESMC_OPTEED_FUNCID_INIT_PID_AND_STACK 92
#define TEESMC_OPTEED_INIT_PID_AND_STACK \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_INIT_PID_AND_STACK)
#define TEESMC_OPTEED_FUNCID_SET_STACK_HASH 93
#define TEESMC_OPTEED_SET_STACK_HASH \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_SET_STACK_HASH)
#define TEESMC_OPTEED_FUNCID_SWITCH_STACK 94
#define TEESMC_OPTEED_SWITCH_STACK \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_SWITCH_STACK)
#define TEESMC_OPTEED_FUNCID_CHECK_PID_AND_STACK 95
#define TEESMC_OPTEED_CHECK_PID_AND_STACK \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_CHECK_PID_AND_STACK)

//取出页表项中物理地址
#define pa_addr(content) (unsigned long long)(content&0x0000fffffffff000)

uintptr_t rkp_process(uint32_t smc_fid,
        u_register_t x1,
        u_register_t x2,
        u_register_t x3,
        u_register_t x4,
        void *cookie,
        void *handle,
        u_register_t flags);

#define SET_USED(a) (a = a | 0x80000000)         //使用|，将第31位设置为1
#define SET_UNUSE(a) (a = a & 0x7FFFFFFF)        //使用&，将第31位设置为0
#define SET_PRE_INDEX(a,b) (a = (a & 0xFFFF8000) | (b))
#define SET_NEXT_INDEX(a,b) (a = (a & 0xC0007FFF) | ((b) << 15))
#define GET_USED(a) (a >> 31)
#define GET_PRE_INDEX(a) (a & 0x00007FFF)
#define GET_NEXT_INDEX(a) ((a & 0x3FFF8000) >> 15)
#define INDEXNULL 0x7FFF
#define PTUSED 1
#define PTUNUSED 0
#define POOLSZIE 8192
#define RKP_PAGE_SIZE 4096

//初始化页表
uintptr_t rkp_pagetable_manange_init(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);
//分配页表
uintptr_t rkp_pagetable_manange_get_a_pagetable(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);
//释放页表
uintptr_t rkp_pagetable_manange_release_a_pagetable(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);
//设置页表内容
uintptr_t rkp_set_pagetable(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);

#define RKP_INS_SIM_STR_IMM_64 0
#define RKP_INS_SIM_STR_IMM_32 1
//指令模拟
#define SIMULATE_STP_XA_XA \
	unsigned long nw_xa = x2; \
            int nw_xa_low = nw_xa; \
            int nw_xa_high = nw_xa >> 32; \
            for (j = 0; j < 2; j++) { \
                for (i = 0; i < 4; i++) { \
                    memset(pa, nw_xa_low, 1); \
                    nw_xa_low = nw_xa_low >> 8; \
                    pa += 1; \
                } \
                for (i = 0; i < 4; i++) { \
                    memset(pa, nw_xa_high, 1); \
                    nw_xa_high = nw_xa_high >> 8; \
                    pa += 1; \
                } \
            }

#define SIMULATE_STP_XA_XB \
	unsigned long nw_xa = x2; \
	unsigned long nw_xb = x3; \
    int nw_xa_l = nw_xa; \
    int nw_xa_h = nw_xa >> 32; \
    int nw_xb_l = nw_xb; \
    int nw_xb_h = nw_xb >> 32; \
    for (i = 0; i < 4; i++) { \
        memset(pa, nw_xa_l, 1); \
        nw_xa_l = nw_xa_l >> 8; \
        pa += 1; \
    } \
    for (i = 0; i < 4; i++) { \
        memset(pa, nw_xa_h, 1); \
        nw_xa_h = nw_xa_h >> 8; \
        pa += 1; \
    } \
    for (i = 0; i < 4; i++) {\
        memset(pa, nw_xb_l, 1); \
        nw_xb_l = nw_xb_l >> 8; \
        pa += 1; \
    } \
    for (i = 0; i < 4; i++) { \
        memset(pa ,nw_xb_h, 1); \
        nw_xb_h = nw_xb_h >> 8; \
        pa += 1; \
    }
	
//指令模拟
uintptr_t rkp_instruction_simulation(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);
//写页表
#define WRITE_ONCE(var, val) \
	(*((volatile typeof(val) *)(&(var))) = (val))
#define idsb(opt)	asm volatile("dsb " #opt : : : "memory")

//清除页表内容
uintptr_t rkp_clear_page(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);
//复制页表
uintptr_t rkp_copy_page(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);
//内存设置
uintptr_t rkp_mem_set(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);
//将Normal World传入的参数赋值给保护区域（内核代码段和只读数据段）的物理起始地址和结束地址
uintptr_t rkp_set_roaddr(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);
//设置双重映射保护的标志
uintptr_t rkp_set_forbid_flag(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);
//设置pxn位
uintptr_t rkp_set_pxn(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);
//只读代码段和只读数据段的保护
uintptr_t pkm_protect_key_code(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);
//selinux保护功能
uintptr_t pkm_selinux(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);

uintptr_t find_task_addr(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);

uintptr_t push_task_addr(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);

uintptr_t set_push_flag(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);

uintptr_t set_pid_and_stack(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);

uintptr_t free_pid_and_stack(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);

uintptr_t init_pid_and_stack(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);

uintptr_t set_stack_hash(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);

uintptr_t check_pid_and_stack(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);

uintptr_t switch_pid_and_stack(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);

void visit_pid_and_stack();

#define GROUP_SIZE 8

#define SHA256_BLOCK_SIZE 32            // SHA256 outputs a 32 byte digest

typedef unsigned char BYTE;             // 8-bit byte
typedef unsigned int  WORD;             // 32-bit word

typedef struct {
	BYTE data[64];
	WORD datalen;
	unsigned long long bitlen;
	WORD state[8];
} SHA256_CTX;

#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32-(b))))
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))

#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

static const WORD k[64] = {
	0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
	0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
	0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
	0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
	0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
	0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
	0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
	0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

void sha256_transform(SHA256_CTX *ctx, const BYTE data[]);

void sha256_init(SHA256_CTX *ctx);

void sha256_update(SHA256_CTX *ctx, const BYTE data[], size_t len);

void sha256_final(SHA256_CTX *ctx, BYTE hash[]);

//受保护进程数组的最大值
#define PROTECTED_TASKADDR_MAXSIZE 10

//保护内核栈的结构体
// typedef struct
// {
//     char state;
//     unsigned long stack;
//     // BYTE hash[SHA256_BLOCK_SIZE];
// }STACK_STRUCT;

typedef struct
{
    short pid;
    char state;
    unsigned long stack;
    // BYTE hash[SHA256_BLOCK_SIZE];
}STACK_STRUCT;

//进程标识的最大值
#define PID_SIZE 2000
//内核栈大小
#define STACK_SIZE 16384

#endif
