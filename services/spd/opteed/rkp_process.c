#include <lib/xlat_tables/xlat_tables_v2.h>
#include <drivers/arm/tzc400.h>
#include <string.h>
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
        case TEESMC_OPTEED_RKP_CLEAR_PAGE:
            result = rkp_clear_page(x1,x2,x3,x4,handle);
            break;
        case TEESMC_OPTEED_RKP_COPY_PAGE:
            result = rkp_copy_page(x1,x2,x3,x4,handle);
            break;
        case TEESMC_OPTEED_RKP_MEM_SET:
            result = rkp_mem_set(x1,x2,x3,x4,handle);
            break;
        case TEESMC_OPTEED_RKP_CFU_PATCH:
            result = rkp_cfu_patch(x1,x2,x3,x4,handle);       
        case TEESMC_OPTEED_RKP_SET_ROADDR:
            result = rkp_set_roaddr(x1,x2,x3,x4,handle);
            break;
        case TEESMC_OPTEED_RKP_SET_FORBID_FLAG:
            result = rkp_set_forbid_flag(x1,x2,x3,x4,handle);
            break;
        case TEESMC_OPTEED_RKP_SET_PXN:
            result = rkp_set_pxn(x1,x2,x3,x4,handle);
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

static unsigned long long ro_start;
static unsigned long long ro_end;
static bool forbid_flag = 0;


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
                (unsigned long long )PTPOOL_END+sizeof(unsigned int)*POOLSZIE-1,TZC_REGION_S_RDWR,0x83038303);
    ALLOCED_PAGE_NUM = 0;
    POOLINITED = 1;
    result = 0;
    ERROR("PTM Initialized start: 0x%016llx end: 0x%016llx\n",(unsigned long long )PTPOOL, (unsigned long long )PTPOOL_END+sizeof(unsigned int)*POOLSZIE-1);
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

void Forbid_double_mapping(unsigned long content)
{
    // char rw = content & PTE_RDONLY;
    char rw = content & 0x0000000000000080;
    unsigned long pa_addr = pa_addr(content);
    // ERROR("content:0x%016llx\n", (unsigned long long)content);
    
    if(forbid_flag == 1 && pa_addr>=ro_start && pa_addr<=ro_end-4096 && rw == 0)
    {
        ERROR("Illegal mapping!\n");
        tzc_configure_region((uint32_t)0x3,(uint8_t)4U,0,(unsigned long long)0xfffffffff,TZC_REGION_S_NONE,0);
    }
	// if(pa_addr>=ro_start && pa_addr<=ro_end-4096 && pa_map_bit(pa_addr) == 1)
	// {
	// 	printf("double mapping!\n");
    //     // dump_stack();
	// }
	// else if(pa_addr>=ro_start && pa_addr<=ro_end-4096)
	// {
	// 	pa_map_bit_fill(pa_addr);
	// 	printf("pa_map_loc:0x%016llx\n",(unsigned long long)pa_map_loc(pa_addr));
	// }
	return;
}

uintptr_t rkp_set_pagetable(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    unsigned long result = -1;
    //unsigned long pagetable_type = x1;
    unsigned long * target = (unsigned long *)x2;
    unsigned long content = x3;
    Forbid_double_mapping(content);
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
    unsigned long instruction = x1;
    unsigned long phys_addr = x4;
    NOTICE("rkp_instruction_simulation | inst: 0x%08lx, write_phys_addr: 0x%016lx\n", instruction, phys_addr);
    result = 0;
    char *pa = (char *)phys_addr;
    int i, j;

    switch(instruction) {
        // instruction in __memset()
        case 0x39000107: {
            NOTICE("get instr: srtb w7, [x8]\n");
            int w7 = x2;
            for (i = 0; i < 1; i++) {
                memset(pa, w7, 1);
                w7 = w7 >> 8;
                pa += 1;
            }
            break;
        }
        case 0x78002507: {
            NOTICE("get instr: strh w7, [x8], #2\n");
            int w7 = x2;
            for (i = 0; i < 2; i++) {
                memset(pa, w7, 1);
                w7 = w7 >> 8;
                pa += 1;
            }
            break;
        }
        case 0xb8004507: {
            NOTICE("get instr: str w7, [x8], #4\n");
            int w7 = x2;
            for (i = 0; i < 4; i++) {
                memset(pa, w7, 1);
                w7 = w7 >> 8;
                pa += 1;
            }
            break;
        }
        case 0xf8008507: {
            NOTICE("get instr: str x7, [x8], #8\n");
            int x7_l = x2;
            int x7_h = x2 >> 32;
            for (i = 0; i < 4; i++) {
                memset(pa, x7_l, 1);
                x7_l = x7_l >> 8;
                pa += 1;
            }
            for (i = 0; i < 4; i++) {
                memset(pa, x7_h, 1);
                x7_h = x7_h >> 8;
                pa += 1;
            }
            break;
        }

        case 0xa9001d07: {
            NOTICE("get instr: stp x7, x7, [x8]\n");
            SIMULATE_STP_XA_XA
            break;
        }
        case 0xa9011d07: {
            NOTICE("get instr: stp x7, x7, [x8, #0x10]\n");
            SIMULATE_STP_XA_XA
            break;
        }
        case 0xa9021d07: {
            NOTICE("get instr: stp x7, x7, [x8, #0x20]\n");
            SIMULATE_STP_XA_XA
            break;
        }
        case 0xa9031d07: {
            NOTICE("get instr: stp x7, x7, [x8, #0x30]\n");
            SIMULATE_STP_XA_XA
            break;
        }
        case 0xd50b7428: {
            NOTICE("get instr: dc zva, x8\n");
            memset(pa, 0, 1024);
            break;
        }

        // instruction in __arch_copy_from_user()
		case 0xf80084c3: {
            NOTICE("get instr: str x3, [x6], #8\n");
            int x3_l = x2;
            int x3_h = x2 >> 32;
            for (i = 0; i < 4; i++) {
                memset(pa, x3_l, 1);
                x3_l = x3_l >> 8;
                pa += 1;
            }
            for (i = 0; i < 4; i++) {
                memset(pa, x3_h, 1);
                x3_h = x3_h >> 8;
                pa += 1;
            }
			break;
        }
		case 0xb80044c3: {
            NOTICE("get instr: str w3, [x6], #4\n");
            int w3 = x2;
            for (i = 0; i < 4; i++) {
                memset(pa, w3, 1);
                w3 = w3 >> 8;
                pa += 1;
            }
			break;
        }
		case 0x780024c3: {
            NOTICE("get instr: strh w3, [x6], #2\n");
            int w3 = x2;
            for (i = 0; i < 2; i++) {
                memset(pa, w3, 1);
                w3 = w3 >> 8;
                pa += 1;
            }
			break;
        }
		case 0x380014c3: {
            NOTICE("get instr: strb w3, [x6], #1\n");
            int w3 = x2;
            for (i = 0; i < 1; i++) {
                memset(pa, w3, 1);
                w3 = w3 >> 8;
                pa += 1;
            }
			break;
        }

        case 0xa88120c7: {
            NOTICE("get instr: stp x7, x8, [x6], #0x10\n");
            SIMULATE_STP_XA_XB
            break;
        }
        case 0xa88128c9: {
            NOTICE("get instr: stp x9, x10, [x6], #0x10\n");
            SIMULATE_STP_XA_XB
            break;
        }
        case 0xa88130cb: {
            NOTICE("get instr: stp x11, x12, [x6], #0x10\n");
            SIMULATE_STP_XA_XB
            break;
        }
        case 0xa88138cd: {
            NOTICE("get instr: stp x13, x14, [x6], #0x10\n");
            SIMULATE_STP_XA_XB
            break;
        }

        default:
            ERROR("unsolved instruction\n");
            result = -1;
            break;        
    }
    SMC_RET1(handle,result);
}

uintptr_t rkp_clear_page(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    void * targetPage_pa_va = (void *)x1;
    memset(targetPage_pa_va, 0, PAGE_SIZE);
    SMC_RET1(handle,0);
}

uintptr_t rkp_copy_page(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    char * to = (char*)x1;
    char* from = (char*)x2;
    unsigned long n = x3;
    memcpy(to,from,n);
    SMC_RET2(handle,0,n);
}
uintptr_t rkp_mem_set(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    void * result;
    result = memset((void*)x1,x2,x3);
    SMC_RET2(handle,0,(unsigned long)result);
}
static int do_cfu_patch_counts = 0;
uintptr_t rkp_cfu_patch(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    if(x1 == 1){
        if(do_cfu_patch_counts == 0){
            tzc_configure_region((uint32_t)0x0,(uint8_t)3U,(unsigned long long )PTPOOL,
                (unsigned long long )PTPOOL_END+sizeof(unsigned int)*POOLSZIE,TZC_REGION_S_RDWR,0x8303);
        }
        do_cfu_patch_counts++;
    }else{
        do_cfu_patch_counts--;
        if(do_cfu_patch_counts == 0){
            tzc_configure_region((uint32_t)0x1,(uint8_t)3U,(unsigned long long )PTPOOL,
                (unsigned long long )PTPOOL_END+sizeof(unsigned int)*POOLSZIE,TZC_REGION_S_RDWR,0x8303);
        }
    }

    SMC_RET0(handle);
}
uintptr_t rkp_set_roaddr(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    ro_start = (unsigned long long)x1;
    ro_end = (unsigned long long)x2;
    ERROR("text_start：%016llx, end:%016llx\n",ro_start,ro_end);
    SMC_RET1(handle,0);
}
uintptr_t rkp_set_forbid_flag(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    forbid_flag = 1;
    SMC_RET1(handle,0);
}

uintptr_t rkp_set_pxn(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    // unsigned long content = (unsigned long)x1;
    // ERROR("pte:0x%016lx\n",x1);
    unsigned long long pte_pa = (unsigned long long)x1;
    ERROR("pte_pa:0x%016llx",pte_pa);
    unsigned long long *content = (unsigned long long *)pte_pa;
    // char *content = (char *)pte_pa;
    ERROR("pte:0x%016llx\n",*content);
    // ERROR("pte:0x%016llx\n",*content);
    unsigned long long p = *content & 0x0020000000000000;
    if (p == 0x0)
    {
        ERROR("pxn not set!\n");
        *content |= 0x0020000000000000;
        // tzc_configure_region((uint32_t)0x3,(uint8_t)4U,0,(unsigned long long)0xfffffffff,TZC_REGION_S_NONE,0);
        // *content |= 0x0020000000000000;
        // x1 |= 0x0020000000000000;
    }
    
    SMC_RET1(handle,0);
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
#include<drivers/delay_timer.h>

static unsigned long long ro_start;//用来存储所保护的内核地址区间的首地址
static unsigned long long ro_end;//用来存储所保护的内核地址区间的尾地址

static char check = 0xff;//初始化check变量，用来表示完整性检查后的结果
uintptr_t pkm_protect_key_code(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    int result=1;//返回至nw的结果，如果是1表示通过完整性检查，如果是0表示没有通过完整性检查   
    char *start = (char *)(ro_start);
    char *end =   (char *)(ro_end);
    //ERROR("rodata start:%016llx,end:%016llx\n",(unsigned long long int)start,(unsigned long long int)end);
    char new_check = *start;
    start++;
    while(start != end)
	{
		new_check ^= *start;//进行异或操作
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
        result=0;//完整性检查出错，将返回至nw的result值设置为0
        printf("there is an error....................");
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
        printf("there is an error....................");
        //tzc_configure_region((uint32_t)0x3,(uint8_t)4U,0,(unsigned long long)0xfffffffff,TZC_REGION_S_NONE,0);
    }
    SMC_RET1(handle,result);
}