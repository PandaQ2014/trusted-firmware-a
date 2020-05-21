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

#define TEESMC_OPTEED_FUNCID_TZC400_SET_READONLY 11
#define TEESMC_OPTEED_TZC400_SET_READONLY \
	TEESMC_OPTEED_RV(TEESMC_OPTEED_FUNCID_TZC400_SET_READONLY)

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
uintptr_t rkp_process(uint32_t smc_fid,
        u_register_t x1,
        u_register_t x2,
        u_register_t x3,
        u_register_t x4,
        void *cookie,
        void *handle,
        u_register_t flags);
uintptr_t tzc400_set_readonly(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);


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

uintptr_t rkp_pagetable_manange_init(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);
uintptr_t rkp_pagetable_manange_get_a_pagetable(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);
uintptr_t rkp_pagetable_manange_release_a_pagetable(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);

uintptr_t rkp_set_pagetable(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);
uintptr_t rkp_instruction_simulation(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle);

#endif