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

#endif