#include <lib/xlat_tables/xlat_tables_v2.h>
#include <drivers/arm/tzc400.h>
#include "rkp_process.h"

uintptr_t rkp_process(uint32_t smc_fid,
        u_register_t x1,
        u_register_t x2,
        u_register_t x3,
        u_register_t x4,
        void *cookie,
        void *handle,
        u_register_t flags){
    uintptr_t result = NULL_PTR;

    switch(smc_fid){
        case TEESMC_OPTEED_TZC400_SET_READONLY:
            result =  tzc400_set_readonly(x1,x2,x3,x4,handle);
            break;
        case TEESMC_OPTEED_RKP_PTM_INIT:
            result = rkp_pagetable_manange_init(x1,x2,x3,x4,handle);
            break;
        case TEESMC_OPTEED_RKP_PTM_GETAPT:
            result = rkp_pagetable_manange_get_a_pagetable(x1,x2,x3,x4,handle);
            break;
        case TEESMC_OPTEED_RKP_PTM_RELEASEAPT:
            result = rkp_pagetable_manange_release_a_pagetable(x1,x2,x3,x4,handle);
            break;
        case TEESMC_OPTEED_RKP_SET_PAGETABLE:
            result = rkp_set_pagetable(x1,x2,x3,x4,handle);
            break;
        case TEESMC_OPTEED_RKP_INSTR_SIMULATION:
            result = rkp_instruction_simulation(x1,x2,x3,x4,handle);
            break;
        case TEESMC_OPTEED_PKM_THREAD:   
            result =pkm_thread(handle);
            break;
        case TEESMC_OPTEED_RKP_SET_ROADDR:
            result = rkp_set_roaddr(x1,x2,x3,x4,handle);
            break;
        case TEESMC_OPTEED_PKM_PROTECT_KEY_CODE:
            result = pkm_protect_key_code(x1,x2,x3,x4,handle);
            break;      
        case TEESMC_OPTEED_PKM_SELINUX:
            result = pkm_selinux(x1,x2,x3,x4,handle);
            break;
        default:
            result = NULL_PTR;
            break;
    }

    return result;
}
uintptr_t tzc400_set_readonly(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    char* phyaddress;
	char* teststr = "Monitor Mode Write Success!";
	size_t teststrlen = 28;
    phyaddress = (char *)x1;
    if(x2 == 1){
        tzc_configure_region((uint32_t)0x1,(uint8_t)3U,(unsigned long long )phyaddress,
                        (unsigned long long )phyaddress+4096,TZC_REGION_S_RDWR,0x8303);
    }else{
        tzc_configure_region((uint32_t)0x0,(uint8_t)3U,0,(unsigned long long)0xfffffffff,0,0);
    }
    ERROR("Config Success!\n");
    ERROR("From NW:%s\n",phyaddress);
    memcpy(phyaddress,teststr,teststrlen);
    ERROR("After SW Write:%s\n",phyaddress);

    SMC_RET1(handle,0x0);
}

static char * PTPOOL;
static char * PTPOOL_END;
static unsigned int * PTMAP;
static unsigned int UNUSED;
static unsigned int USED;
static int POOLINITED = 0;
static int ALLOCED_PAGE_NUM = 0;

uintptr_t rkp_pagetable_manange_init(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    char * pa_start = (char*)x1;
    unsigned long result = -1;
    if(sizeof(void*) != sizeof(u_register_t)){
        result = -1;
        ERROR("sizeof 'void *' does not equal sizeof 'u_register_t'(unsigned long)\n");
        ERROR("more info: sizeof 'unsigned long'-0x%lu, sizeof 'unsigned long long'-0x%lu\n",sizeof(unsigned long),sizeof(unsigned long long));
        goto finished;
    }
    if(x2 != POOLSZIE){
        result = -1;
        ERROR("bad POOLSZIE %lu, not %d\n",x2,POOLSZIE);
        goto finished;
    }
    PTPOOL = pa_start;
    PTPOOL_END = pa_start+POOLSZIE*RKP_PAGE_SIZE;
    PTMAP = (unsigned int *)PTPOOL_END;
    UNUSED = 0;
    USED = INDEXNULL;
    for(int i=0;i<POOLSZIE;i++){
        SET_UNUSE(PTMAP[i]);
        if(i == 0){
          SET_PRE_INDEX(PTMAP[i], INDEXNULL);
        }else{
            SET_PRE_INDEX(PTMAP[i], i-1);
        }
        if(i == POOLSZIE-1){
            SET_NEXT_INDEX(PTMAP[i], INDEXNULL);
        }else{
            SET_NEXT_INDEX(PTMAP[i], i+1);
        }
    }
    tzc_configure_region((uint32_t)0x1,(uint8_t)3U,(unsigned long long )PTPOOL,
                (unsigned long long )PTPOOL_END+sizeof(unsigned int)*POOLSZIE,TZC_REGION_S_RDWR,0x8303);
    ALLOCED_PAGE_NUM = 0;
    POOLINITED = 1;
    result = 0;
    ERROR("PTM Initialized start 0x%016llx 0x%016llx\n",(unsigned long long )PTPOOL, (unsigned long long )PTPOOL_END+sizeof(unsigned int)*POOLSZIE);
    finished:
    SMC_RET1(handle,result);
}

uintptr_t rkp_pagetable_manange_get_a_pagetable(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    unsigned long result = -1;
    char * pa_result = NULL;
    if(POOLINITED == 0){
        result = -1;
        ERROR("page table was uninitialized\n");
        goto finished;
    }
    if(UNUSED != INDEXNULL){
        int temp = UNUSED;
        char * tempbuffer;
        int pre = GET_PRE_INDEX(PTMAP[temp]);
        int next = GET_NEXT_INDEX(PTMAP[temp]);
        SET_USED(PTMAP[temp]);
        if(pre != INDEXNULL){
            SET_NEXT_INDEX(PTMAP[pre], next);
        }else{
            UNUSED = next;
        }
        if(next != INDEXNULL){
            SET_PRE_INDEX(PTMAP[next], pre);
        }
        if(USED != INDEXNULL){
            SET_PRE_INDEX(PTMAP[USED], temp);
        }
        SET_NEXT_INDEX(PTMAP[temp], USED);
        SET_PRE_INDEX(PTMAP[temp], INDEXNULL);
        USED = temp;
        tempbuffer = PTPOOL + temp * RKP_PAGE_SIZE;
        for(int i = 0;i< RKP_PAGE_SIZE;i++){
            tempbuffer[i] = 0;
        }
        result = 0;
        ALLOCED_PAGE_NUM++;
        pa_result = PTPOOL + temp * RKP_PAGE_SIZE;
        ERROR("get a page %d 0x%016lx\n",ALLOCED_PAGE_NUM,(unsigned long)pa_result);
        goto finished;
    }else{
        result = -1;
        ERROR("have no unused page table\n");
        goto finished;
    }

    finished:
    SMC_RET2(handle,result,(unsigned long)pa_result);
}

uintptr_t rkp_pagetable_manange_release_a_pagetable(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    unsigned long result = -1;
    char * target = (char *)x1;
    if(POOLINITED == 0){
        result = -1;
        ERROR("page table was uninitialized\n");
        goto finished;
    }
    int targetIndex =(int)((target - PTPOOL) / RKP_PAGE_SIZE);
    if(-1<targetIndex && POOLSZIE > targetIndex && GET_USED(PTMAP[targetIndex]) == PTUSED){
        SET_UNUSE(PTMAP[targetIndex]);
        int pre = GET_PRE_INDEX(PTMAP[targetIndex]);
        int next = GET_NEXT_INDEX(PTMAP[targetIndex]);
        if(pre != INDEXNULL){
            SET_NEXT_INDEX(PTMAP[pre], next);
        }else{
            USED = next;
        }
        if(next != INDEXNULL){
            SET_PRE_INDEX(PTMAP[next], pre);
        }
        
        if(UNUSED != INDEXNULL){
            SET_PRE_INDEX(PTMAP[UNUSED], targetIndex);
        }
        SET_NEXT_INDEX(PTMAP[targetIndex], UNUSED);
        SET_PRE_INDEX(PTMAP[targetIndex], INDEXNULL);
        UNUSED = targetIndex;
        ALLOCED_PAGE_NUM--;
         ERROR("release a page %d 0x%016lx\n",ALLOCED_PAGE_NUM,(unsigned long)target);
        result = 0;
        goto finished;
        
    }else{
        result = -1;
        ERROR("bad target page to release\n");
        goto finished;
    }
    finished:
    SMC_RET1(handle,result);
}

uintptr_t rkp_set_pagetable(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    unsigned long result = -1;
    //unsigned long pagetable_type = x1;
    unsigned long * target = (unsigned long *)x2;
    unsigned long content = x3;
    WRITE_ONCE(*target,content);
    idsb(ishst);
    result = 0;
    // switch(pagetable_type){
    //     case 0:
    //         ERROR("write PTE to 0x%016lx 0x%016lx\n",x2,x3);
    //         break;
    //     case 1:
    //         ERROR("write PMD to 0x%016lx 0x%016lx\n",x2,x3);
    //         break;
    //     case 2:
    //         ERROR("write PUD to 0x%016lx 0x%016lx\n",x2,x3);
    //         break;
    //     case 3:
    //         ERROR("write PGD to 0x%016lx 0x%016lx\n",x2,x3);
    //         break;
    //     default:
    //         ERROR("write to 0x%016lx 0x%016lx\n",x2,x3);
    //         break;        
    // }
    SMC_RET1(handle,result);
}

uintptr_t rkp_instruction_simulation(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    unsigned long result = -1;
    unsigned long instruction_mark = x1;
    result = 0;
    switch(instruction_mark){

        default:
            ERROR("unsolved instruction\n");
            result = -1;
            break;        
    }
    SMC_RET1(handle,result);
}

#include<context.h>
#include <arch_helpers.h>
#include <bl31/bl31.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <common/runtime_svc.h>
#include <lib/el3_runtime/context_mgmt.h>
#include <plat/common/platform.h>
#include <tools_share/uuid.h>

uint64_t ttbr1_value=0xffffffffffffffff; //全局变量，为了存储首次出现的ttbr1的值

#include<context.h>
uintptr_t pkm_thread(void *handle){
    cpu_context_t *ctx;//cpu上下文，为了去取出ttbr1_el1寄存器的值。
    ctx=cm_get_context(NON_SECURE);//取出非安全态（即NW）的cpu状态
    assert(ctx != NULL);
    el1_sys_regs_t *state;//el1系统寄存器状态
    state=get_sysregs_ctx(ctx);
    int result=1; //返回结果，make nw panic
    uint64_t ttbr1= read_ctx_reg(state, CTX_TTBR1_EL1);//从NW中取出的ttbr1_el1的值
    uint64_t ttbr1_result=0x0000000000000000;//对比结果，即每次异或后的结果
    printf("TTBR1寄存器原本的内容0x%016llx\n",ttbr1);
    ttbr1 &= 0x0000ffffffffffff;//与操作，为了将前四位asid全部置换为0
    printf("TTBR1寄存器的内容0x%016llx\n",ttbr1);
    /*   判断全局变量是否还未被赋值，若未赋值，则将第一次的ttbr1寄存器值赋予它*/
    if(ttbr1_value==0xffffffffffffffff){
        ttbr1_value=ttbr1;
    }
    printf("TTBR1_value内容0x%016llx\n",ttbr1_value);
    ttbr1_result=ttbr1^ttbr1_value;
    if(ttbr1_result==0x0000000000000000){
        printf("安全\n");
    }
    else{
        printf("不安全0x%016llx\n",ttbr1_result);
        //tzc_configure_region((uint32_t)0x3,(uint8_t)4U,0,(unsigned long long)0xfffffffff,TZC_REGION_S_NONE,0);
        result=0;
        printf("检测到ttbr1_el1寄存器被修改，将强制封锁系统的内存读写权限");
    }   
    SMC_RET1(handle,result);
}



static unsigned long long ro_start;
static unsigned long long ro_end;




uintptr_t rkp_set_roaddr(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    ro_start = (unsigned long long)x1;
    ro_end = (unsigned long long)x2;
    ERROR("text_start£º%016llx, end:%016llx\n",ro_start,ro_end);
    SMC_RET1(handle,1);
}



static char check = 0xff;
uintptr_t pkm_protect_key_code(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    // if(ro_start == NULL)
    // {
    //     ro_start = (char *)x1;
    //     ro_end = (char *)x1 + x2;
    //     ERROR("rodata start:%016llx,end:%016llx\n",(unsigned long long int)ro_start,(unsigned long long int)ro_end);
    //     SMC_RET1(handle,0);
    // }
    // char *start = (char *)x1;
    // char *end = (char *)x1 + x2;
    // ERROR("%d",SEPARATE_CODE_AND_RODATA);
    // ERROR("%016llx",(unsigned long long)__RODATA_START__);
    //ERROR("rodata start:%016llx,end:%016llx\n",(unsigned long long int)start,(unsigned long long int)end);
    int result=1;    
    char *start = (char *)ro_start;
    char *end = (char *)ro_end;
    char new_check = *start;
    start++;
    while(start != end)
	{
		new_check ^= *start;
		start++;
	}
    ERROR("new_check:0x%02x\n",new_check);
	//ERROR("check:0x%02x\n",check);
	if(check == 0xff)
	{
		check = new_check;
		ERROR("change check:0x%02x\n",check);
	}
	else if (check == new_check)
	{
		ERROR("rodata safe!\n");
	}
	else 
	{
		ERROR("rodata error!\n");
        //tzc_configure_region((uint32_t)0x3,(uint8_t)4U,0,(unsigned long long)0xfffffffff,TZC_REGION_S_NONE,0);
        result=0;
	}
    SMC_RET1(handle,result);
}

static unsigned long long int *enabled_addr = NULL;
static bool *enforcing_addr = NULL;

uintptr_t pkm_selinux(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    int result=1;    
    if(enabled_addr == NULL)
    {
        enabled_addr = (unsigned long long *)x1;
        enforcing_addr = (bool *)x2;
        ERROR("selinux_enabled:%lld, addr:%016llx\n",*enabled_addr,(unsigned long long int)enabled_addr);
        // ERROR("selinux_enforcing: addr:%016llx\n",(unsigned long long int)enforcing_addr);
        ERROR("selinux_enforcing:%x, addr:%016llx\n",*enforcing_addr,(unsigned long long int)enforcing_addr);
        SMC_RET1(handle,0);
    }
    if(*enabled_addr == 1 && *enforcing_addr == 1)
    {
        ERROR("selinux safa!\n");
    }
    else 
    {
        ERROR("selinux unsafe!\n");
        result=0;
        //tzc_configure_region((uint32_t)0x3,(uint8_t)4U,0,(unsigned long long)0xfffffffff,TZC_REGION_S_NONE,0);
    }
    SMC_RET1(handle,result);
}