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