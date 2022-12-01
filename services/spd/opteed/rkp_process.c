#include <lib/xlat_tables/xlat_tables_v2.h>
#include <drivers/arm/tzc400.h>
#include <string.h>
#include <arch_helpers.h>
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
    //根据smc指令id执行对应功能
    switch(smc_fid){
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
        case TEESMC_OPTEED_KILL_HOOK:
            result = find_task_addr(x1,x2,x3,x4,handle);
            break;
        case TEESMC_OPTEED_PUSH_TASKADDR:
            result = push_task_addr(x1,x2,x3,x4,handle);
            break;
        case TEESMC_OPTEED_SET_PUSH_TASKADDR_FLAG:
            result = set_push_flag(x1,x2,x3,x4,handle);
            break;
        case TEESMC_OPTEED_SET_PID_AND_STACK:
            result = set_pid_and_stack(x1,x2,x3,x4,handle);
            break;
        case TEESMC_OPTEED_FREE_PID_AND_STACK:
            result = free_pid_and_stack(x1,x2,x3,x4,handle);
            break;
        case TEESMC_OPTEED_INIT_PID_AND_STACK:
            printf("teec init\n");
            result = init_pid_and_stack(x1,x2,x3,x4,handle);
            break;
        // case TEESMC_OPTEED_FUNCID_SET_STACK_HASH:
        //     result = set_stack_hash(x1,x2,x3,x4,handle);
        //     break;
        case TEESMC_OPTEED_SWITCH_STACK:
            result = switch_pid_and_stack(x1,x2,x3,x4,handle);
            break;
        // case TEESMC_OPTEED_FUNCID_CHECK_PID_AND_STACK:
        //     result = check_pid_and_stack(x1,x2,x3,x4,handle);
        //     break;
        case TEESMC_OPTEED_VISIT_STACK_STRUCT:
            result = visit_stack_struct(x1,x2,x3,x4,handle);
            break;
        case TEESMC_OPTEED_SET_CRED:
            result = set_cred(x1,x2,x3,x4,handle);
            break;
        case TEESMC_OPTEED_CHECK_CRED:
            result = check_cred(x1,x2,x3,x4,handle);
            break;
        case TEESMC_OPTEED_FREE_CRED:
            result = free_cred(x1,x2,x3,x4,handle);
            break;
        default:
            result = NULL_PTR;
            break;
    }
    return result;
}

static char * PTPOOL;//安全内存起始位置
static char * PTPOOL_END;//安全内存结束位置
static unsigned int * PTMAP;
static unsigned int UNUSED;
static unsigned int USED;
static int POOLINITED = 0;//安全内存区域是否初始化
static int ALLOCED_PAGE_NUM = 0;

static char * SSPOOL;
static char * SSPOOL_END;

static unsigned long long ro_start;//内核代码段和只读数据段的起始地址
static unsigned long long ro_end;//内核代码段和只读数据段的结束地址
static bool forbid_flag = 0;//双重映射保护功能的标志

static STACK_STRUCT * stack_struct_table;
static int stack_struct_table_size = 0;

// static CRED_STRUCT cred_struct_table[PID_SIZE];
static CRED_STRUCT *cred_struct_table;
static int cred_struct_table_size = 0;

//初始化页表
uintptr_t rkp_pagetable_manange_init(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    char * pa_start = (char*)x1;//获取起始地址
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
    // SSPOOL = PTPOOL_END+sizeof(unsigned int)*POOLSZIE;
    // SSPOOL_END = SSPOOL + sizeof(STACK_STRUCT)*PID_SIZE;

    SSPOOL = (char*)x3;
    SSPOOL_END = SSPOOL + sizeof(STACK_STRUCT)*PID_SIZE + sizeof(CRED_STRUCT)*PID_SIZE;

    UNUSED = 0;
    USED = INDEXNULL;
    //初始化安全内存双向链表
    for(int i=0;i<POOLSZIE;i++){
        SET_UNUSE(PTMAP[i]);//设置所有页表为未使用
        //设置双向链表的前驱和后继
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

    stack_struct_table = (STACK_STRUCT *)SSPOOL; 
    for(int i = 0; i < PID_SIZE; ++i)
    {
        stack_struct_table[i].pid = 0;
        stack_struct_table[i].state = 0;
        stack_struct_table[i].stack = 0;
    }

    cred_struct_table = (CRED_STRUCT *) (SSPOOL + sizeof(STACK_STRUCT) * PID_SIZE);
    for(int i = 0; i < PID_SIZE; ++i)
    {
        cred_struct_table[i].pid = 0;
        cred_struct_table[i].task_struct_addr = 0;
        cred_struct_table[i].cred_addr = 0;
        cred_struct_table[i].pgd = 0;
        cred_struct_table[i].uid = 10000;
        cred_struct_table[i].gid = 10000;
        cred_struct_table[i].suid = 10000;
        cred_struct_table[i].sgid = 10000;
        cred_struct_table[i].euid = 10000;
        cred_struct_table[i].egid = 10000;
    }

    tzc_configure_region((uint32_t)0x1,(uint8_t)3U,(unsigned long long )PTPOOL,
                (unsigned long long )PTPOOL_END+sizeof(unsigned int)*POOLSZIE-1,TZC_REGION_S_RDWR,0x83038303);
    // tzc_configure_region((uint32_t)0x1,(uint8_t)3U,(unsigned long long )PTPOOL,
    //             (unsigned long long )PTPOOL_END+sizeof(unsigned int)*POOLSZIE + sizeof(STACK_STRUCT) * PID_SIZE -1,TZC_REGION_S_RDWR,0x83038303);
    // tzc_configure_region((uint32_t)0x1,(uint8_t)3U,(unsigned long long )PTPOOL,
    //             (unsigned long long )SSPOOL_END -1,TZC_REGION_S_RDWR,0x83038303);
    tzc_configure_region((uint32_t)0x1,(uint8_t)3U,(unsigned long long )SSPOOL,
                (unsigned long long )SSPOOL_END -1,TZC_REGION_S_RDWR,0x8303);

    ALLOCED_PAGE_NUM = 0;
    POOLINITED = 1;
    result = 0;
    ERROR("PTM Initialized start: 0x%016llx end: 0x%016llx\n",(unsigned long long )PTPOOL, (unsigned long long )PTPOOL_END+sizeof(unsigned int)*POOLSZIE-1);
    ERROR("SSPOOL Initialized start: 0x%016llx end: 0x%016llx\n",(unsigned long long )SSPOOL, (unsigned long long )SSPOOL_END - 1);

    finished:
    SMC_RET1(handle,result);
}

//分配页表
uintptr_t rkp_pagetable_manange_get_a_pagetable(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    unsigned long result = -1;
    char * pa_result = NULL;
    //未进行初始化
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
        SET_USED(PTMAP[temp]);//设置页表已使用
        //更新安全内存区域中双向链表的前驱后继关系
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
        goto finished;
    }else{
        result = -1;
        ERROR("have no unused page table\n");
        goto finished;
    }

    finished:
    SMC_RET2(handle,result,(unsigned long)pa_result);
}

//释放页表
uintptr_t rkp_pagetable_manange_release_a_pagetable(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    unsigned long result = -1;
    char * target = (char *)x1;
    //未初始化
    if(POOLINITED == 0){
        result = -1;
        ERROR("page table was uninitialized\n");
        goto finished;
    }
    int targetIndex =(int)((target - PTPOOL) / RKP_PAGE_SIZE);
    if(-1<targetIndex && POOLSZIE > targetIndex && GET_USED(PTMAP[targetIndex]) == PTUSED){
        //更新安全内存中双向链表的前驱后继关系
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

//禁止双重映射
void Forbid_double_mapping(unsigned long content)
{
    //读取页表项中的只读权限
    char rw = content & 0x0000000000000080;
    //读取页表项中的物理地址
    unsigned long pa_addr = pa_addr(content);
    //如果页表项中的物理地址在保护区域（内核代码段和只读数据段）之间，且不为只读，则打出log并panic
    if(pa_addr>=ro_start && pa_addr<=ro_end-4096 && rw == 0)
    {
        ERROR("Illegal mapping!\n");
        panic();
    }
	return;
}

//设置页表内容
uintptr_t rkp_set_pagetable(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    unsigned long result = -1;
    //unsigned long pagetable_type = x1;
    unsigned long * target = (unsigned long *)x2;
    unsigned long content = x3;
    //如果开启防止双重映射标志为1，则进一步防止双重映射
    if(forbid_flag == 1)
    {
        Forbid_double_mapping(content);
    }
    WRITE_ONCE(*target,content);//写入页表内容
    idsb(ishst);
    result = 0;
    SMC_RET1(handle,result);
}

//指令模拟
uintptr_t rkp_instruction_simulation(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    unsigned long result = -1;
    unsigned long instruction = x1;
    unsigned long phys_addr = x4;
    result = 0;
    char *pa = (char *)phys_addr;
    int i, j;
    //检查双重映射
    unsigned long content1 = x2;
    unsigned long content2 = x3;
    //如果开启防止双重映射标志为1，则进一步防止双重映射
    if(forbid_flag == 1)
    {
        Forbid_double_mapping(content1);
        Forbid_double_mapping(content2);
    }
    switch(instruction) {
        // instruction in __memset()
        //模拟srtb w7, [x8]指令
        case 0x39000107: {
            int w7 = x2;
            for (i = 0; i < 1; i++) {
                memset(pa, w7, 1);
                w7 = w7 >> 8;
                pa += 1;
            }
            break;
        }
        //模拟 strh w7, [x8], #2指令
        case 0x78002507: {
            int w7 = x2;
            for (i = 0; i < 2; i++) {
                memset(pa, w7, 1);
                w7 = w7 >> 8;
                pa += 1;
            }
            break;
        }
        //模拟 str w7, [x8], #4指令
        case 0xb8004507: {
            int w7 = x2;
            for (i = 0; i < 4; i++) {
                memset(pa, w7, 1);
                w7 = w7 >> 8;
                pa += 1;
            }
            break;
        }
        //模拟 str x7, [x8], #8指令
        case 0xf8008507: {
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
        //模拟 stp x7, x7, [x8]指令
        case 0xa9001d07: {
            SIMULATE_STP_XA_XA
            break;
        }
        //模拟 stp x7, x7, [x8, #0x10]指令
        case 0xa9011d07: {
            SIMULATE_STP_XA_XA
            break;
        }
        //模拟 stp x7, x7, [x8, #0x20]指令
        case 0xa9021d07: {
            SIMULATE_STP_XA_XA
            break;
        }
        //模拟 stp x7, x7, [x8, #0x30]指令
        case 0xa9031d07: {
            SIMULATE_STP_XA_XA
            break;
        }
        //模拟 dc zva, x8指令
        case 0xd50b7428: {
            memset(pa, 0, 1024);
            break;
        }

        // instruction in __arch_copy_from_user()
        //模拟  str x3, [x6], #8指令
         case 0xf80084c3: {
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
        //模拟 str w3, [x6], #4指令
        case 0xb80044c3: {
            int w3 = x2;
            for (i = 0; i < 4; i++) {
                memset(pa, w3, 1);
                w3 = w3 >> 8;
                pa += 1;
            }
			break;
        }
        //模拟 strh w3, [x6], #2指令
        case 0x780024c3: {
            int w3 = x2;
            for (i = 0; i < 2; i++) {
                memset(pa, w3, 1);
                w3 = w3 >> 8;
                pa += 1;
            }
			break;
        }
        //模拟 strb w3, [x6], #1指令
        case 0x380014c3: {
            int w3 = x2;
            for (i = 0; i < 1; i++) {
                memset(pa, w3, 1);
                w3 = w3 >> 8;
                pa += 1;
            }
			break;
        }
        //模拟stp x7, x8, [x6], #0x10指令
        case 0xa88120c7: {
            SIMULATE_STP_XA_XB
            break;
        }
        //模拟stp x9, x10, [x6], #0x10指令
        case 0xa88128c9: {
            SIMULATE_STP_XA_XB
            break;
        }
        //模拟 stp x11, x12, [x6], #0x10指令
        case 0xa88130cb: {
            SIMULATE_STP_XA_XB
            break;
        }
        //模拟 stp x13, x14, [x6], #0x10指令
        case 0xa88138cd: {
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

//清除页表内容
uintptr_t rkp_clear_page(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    void * targetPage_pa_va = (void *)x1;
    memset(targetPage_pa_va, 0, PAGE_SIZE);
    SMC_RET1(handle,0);
}

//复制页表
uintptr_t rkp_copy_page(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    char * to = (char*)x1;
    char* from = (char*)x2;
    unsigned long n = x3;
    //内存拷贝
    memcpy(to,from,n);
    SMC_RET2(handle,0,n);
}

//内存设置
uintptr_t rkp_mem_set(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    void * result;
    result = memset((void*)x1,x2,x3);
    SMC_RET2(handle,0,(unsigned long)result);
}

//将Normal World传入的参数赋值给保护区域（内核代码段和只读数据段）的物理起始地址和结束地址
uintptr_t rkp_set_roaddr(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    ro_start = (unsigned long long)x1;
    ro_end = (unsigned long long)x2;
    ERROR("text_start: %016llx, end:%016llx\n",ro_start,ro_end);
    SMC_RET1(handle,0);
}
//设置双重映射保护的标志
uintptr_t rkp_set_forbid_flag(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    forbid_flag = 1;
    SMC_RET1(handle,0);
}

//设置pxn位
uintptr_t rkp_set_pxn(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    //取出Normal World传来的物理地址
    unsigned long long pte_pa = (unsigned long long)x1;
    //根据物理地址读取对应页表项内容
    unsigned long long *content = (unsigned long long *)pte_pa;
    //读取PXN位
    unsigned long long p = *content & 0x0020000000000000;
    //如果PXN位为0则打出log并将其置为1
    if (p == 0x0)
    {
        ERROR("pxn not set!\n");
        *content |= 0x0020000000000000;
    }
    
    SMC_RET1(handle,0);
}


//PKM

static int check_flag = 0;
static BYTE check[GROUP_SIZE][SHA256_BLOCK_SIZE];

void sha256_transform(SHA256_CTX *ctx, const BYTE data[])
{
	WORD a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

	for (i = 0, j = 0; i < 16; ++i, j += 4)
		m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
	for ( ; i < 64; ++i)
		m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

	a = ctx->state[0];
	b = ctx->state[1];
	c = ctx->state[2];
	d = ctx->state[3];
	e = ctx->state[4];
	f = ctx->state[5];
	g = ctx->state[6];
	h = ctx->state[7];

	for (i = 0; i < 64; ++i) {
		t1 = h + EP1(e) + CH(e,f,g) + k[i] + m[i];
		t2 = EP0(a) + MAJ(a,b,c);
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	ctx->state[0] += a;
	ctx->state[1] += b;
	ctx->state[2] += c;
	ctx->state[3] += d;
	ctx->state[4] += e;
	ctx->state[5] += f;
	ctx->state[6] += g;
	ctx->state[7] += h;
}

void sha256_init(SHA256_CTX *ctx)
{
	ctx->datalen = 0;
	ctx->bitlen = 0;
	ctx->state[0] = 0x6a09e667;
	ctx->state[1] = 0xbb67ae85;
	ctx->state[2] = 0x3c6ef372;
	ctx->state[3] = 0xa54ff53a;
	ctx->state[4] = 0x510e527f;
	ctx->state[5] = 0x9b05688c;
	ctx->state[6] = 0x1f83d9ab;
	ctx->state[7] = 0x5be0cd19;
}

void sha256_update(SHA256_CTX *ctx, const BYTE data[], size_t len)
{
	WORD i;
    for (i = 0; i < len; i+=GROUP_SIZE) {
		ctx->data[ctx->datalen] = data[i];
		ctx->datalen++;
		if (ctx->datalen == 64) {
			sha256_transform(ctx, ctx->data);
			ctx->bitlen += 512;
			ctx->datalen = 0;
		}
	}
}

void sha256_update_stack(SHA256_CTX *ctx, const BYTE data[], size_t len)
{
	WORD i;
    for (i = 0; i < len; i += 1) {
		ctx->data[ctx->datalen] = data[i];
		ctx->datalen++;
		if (ctx->datalen == 64) {
			sha256_transform(ctx, ctx->data);
			ctx->bitlen += 512;
			ctx->datalen = 0;
		}
	}
}

void sha256_final(SHA256_CTX *ctx, BYTE hash[])
{
	WORD i;

	i = ctx->datalen;

	if (ctx->datalen < 56) {
		ctx->data[i++] = 0x80;
		while (i < 56)
			ctx->data[i++] = 0x00;
	}
	else {
		ctx->data[i++] = 0x80;
		while (i < 64)
			ctx->data[i++] = 0x00;
		sha256_transform(ctx, ctx->data);
		memset(ctx->data, 0, 56);
	}

	ctx->bitlen += ctx->datalen * 8;
	ctx->data[63] = ctx->bitlen;
	ctx->data[62] = ctx->bitlen >> 8;
	ctx->data[61] = ctx->bitlen >> 16;
	ctx->data[60] = ctx->bitlen >> 24;
	ctx->data[59] = ctx->bitlen >> 32;
	ctx->data[58] = ctx->bitlen >> 40;
	ctx->data[57] = ctx->bitlen >> 48;
	ctx->data[56] = ctx->bitlen >> 56;
	sha256_transform(ctx, ctx->data);

	for (i = 0; i < 4; ++i) {
		hash[i]      = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 4]  = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 8]  = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0x000000ff;
	}
}

long rand(long time)
{
    return (((time * 214013L + 2531011L) >> 16) & 0x7fff);
}

//只读代码段和只读数据段的保护
uintptr_t pkm_protect_key_code(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    int result=1;//返回至nw的结果，如果是1表示通过完整性检查，如果是0表示没有通过完整性检查   
    char *start = (char *)(ro_start);
    char *end =   (char *)(ro_end);
    //ERROR("rodata start:%016llx,end:%016llx\n",(unsigned long long int)start,(unsigned long long int)end);

    SHA256_CTX ctx;
    BYTE new_check[SHA256_BLOCK_SIZE];

    if(check_flag == 0)
    {
        for(int i = 0; i < GROUP_SIZE; i++)
        {
            sha256_init(&ctx);
	        sha256_update(&ctx, (BYTE *)(start + i), (long long)end-(long long)start-i);
	        sha256_final(&ctx, new_check);
            memcpy(check[i],new_check,SHA256_BLOCK_SIZE);
        }
        check_flag = 1;
    }
    else
    {
        long time = read_cntpct_el0();
        int r = rand(time) % GROUP_SIZE;

        sha256_init(&ctx);
	    sha256_update(&ctx, (BYTE *)(start + r), (long long)end-(long long)start-r);
	    sha256_final(&ctx, new_check);

        if (memcmp(check[r],new_check,SHA256_BLOCK_SIZE) == 0)
        {
            ERROR("rodata safe!\n");
        }
        else 
        {
            ERROR("rodata error!\n");
            result = 0;//完整性检查出错，将返回至nw的result值设置为0
            printf("there is an error....................");
        }
    }
    SMC_RET1(handle,result);
}

static unsigned long long int *enabled_addr = NULL;
static bool *enforcing_addr = NULL;
//selinux保护功能
uintptr_t pkm_selinux(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle){
    int result=1;    
    if(enabled_addr == NULL)
    {
        enabled_addr = (unsigned long long *)x1;
        enforcing_addr = (bool *)x2;
        ERROR("selinux_enabled:%lld, addr:%016llx\n",*enabled_addr,(unsigned long long int)enabled_addr);
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
        result = 0;
        printf("there is an error....................");
    }
    // visit_pid_and_stack();
    SMC_RET1(handle,result);
}

//所保护进程数组
static unsigned long long protected_taskaddr[PROTECTED_TASKADDR_MAXSIZE] = {0};
//数组大小
static short protected_taskaddr_size = 0;
//是否能再添加保护进程的标志
static bool push_taskaddr_flag = true;

//遍历保护的进程
void visit_pid()
{
    for(int i = 0; i < protected_taskaddr_size; i++)
    {
        printf("0x%016llx ", protected_taskaddr[i]);
    }
    printf("\n");
}

//查找传入的进程标识是否在受保护进程数组中
uintptr_t find_task_addr(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle)
{
    // visit_pid();
    for(int i = 0; i < protected_taskaddr_size; i++)
    {
        if(protected_taskaddr[i] == x1)
            SMC_RET1(handle, 1);
    }
    SMC_RET1(handle, 0);
}

//将传入的进程标识添加到受保护进程数组中
uintptr_t push_task_addr(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle)
{
    if(!push_taskaddr_flag || protected_taskaddr_size >= PROTECTED_TASKADDR_MAXSIZE)
        SMC_RET1(handle, -1);
    protected_taskaddr[protected_taskaddr_size] = x1;
    protected_taskaddr_size++;
    // visit_pid();
    SMC_RET1(handle, 1);
}

//将能否添加受保护进程的flag设置为false 不再可以添加进程
uintptr_t set_push_flag(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle)
{
    push_taskaddr_flag = false;
    SMC_RET1(handle, 1);
}


// 保护内核栈的结构体
// static STACK_STRUCT * stack_struct_table;
// static STACK_STRUCT stack_struct_table[PID_SIZE];
// static int stack_struct_table_size = 0;
// static short pid_array[PID_SIZE];
// static unsigned long long stack_struct_table[PID_SIZE];


uintptr_t visit_stack_struct(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle)
{
    for(int i = 0; i < stack_struct_table_size; i++)
    {
        // if(stack_struct_table[i].state == 1)
        //     ERROR("pid: %d, stack:0x%016lx\n", i, stack_struct_table[i].stack);
        ERROR("pid:%d, state:%d, stack:0x%016lx\n", stack_struct_table[i].pid, (int)stack_struct_table[i].state, stack_struct_table[i].stack);
    }
    SMC_RET1(handle, 1);
}

void visit_pid_and_stack()
{
    ERROR("size:%d \n", stack_struct_table_size);
    for(int i = 0; i < stack_struct_table_size; i++)
    {
        // if(stack_struct_table[i].state == 1)
        //     ERROR("pid: %d, stack:0x%016lx\n", i, stack_struct_table[i].stack);
        ERROR("pid:%d, state:%d, stack:0x%016lx\n", stack_struct_table[i].pid, (int)stack_struct_table[i].state, stack_struct_table[i].stack);
    }
}

//设置进程标识对应的内核栈基地址
uintptr_t set_pid_and_stack(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle)
{
    if ((short)x1 == 0)
    {
        for(int i = 0; i < stack_struct_table_size; ++i)
        {
            if(stack_struct_table[i].pid == 0)
            {
                tzc_configure_region((uint32_t)0x0,(uint8_t)3U,stack_struct_table[i].stack,stack_struct_table[i].stack + STACK_SIZE - 1,0,0);
                stack_struct_table[i].stack = (unsigned long)x2;
                SMC_RET1(handle, 1);
            }
        }
    }
    if (stack_struct_table_size >= PID_SIZE)
    {
        ERROR("stack_struct_table_size >= PID_SIZE");
        panic();
    }
    stack_struct_table[stack_struct_table_size].state = 1;
    stack_struct_table[stack_struct_table_size].pid = (short)x1;
    stack_struct_table[stack_struct_table_size].stack = (unsigned long)x2;
    // char *start = (char *)stack_struct_table[stack_struct_table_size].stack;
    // SHA256_CTX ctx;
    // BYTE new_hash[SHA256_BLOCK_SIZE];
    // sha256_init(&ctx);
	// sha256_update_stack(&ctx, (BYTE *)start, STACK_SIZE - 1);
    // sha256_final(&ctx, new_hash);
    // memcpy(stack_struct_table[stack_struct_table_size].hash, new_hash, SHA256_BLOCK_SIZE);
    stack_struct_table_size++;
    // ERROR("size: %d, set  pid: %d, state:%d, stack:0x%016lx\n", stack_struct_table_size, stack_struct_table[stack_struct_table_size - 1].pid, 
    //     (int)stack_struct_table[stack_struct_table_size - 1].state, stack_struct_table[stack_struct_table_size - 1].stack);
    // for(int i = 0; i < SHA256_BLOCK_SIZE; ++i)
    // {
    //     printf("%02x", stack_struct_table[stack_struct_table_size - 1].hash[i]);
    // }
    // printf("\n");
    // stack_struct_table[x1].state = 1;
    // stack_struct_table[x1].stack = (unsigned long)x2;
    // ERROR("set  pid: %d, state:%d, stack:0x%016lx\n", (int)x1, (int)stack_struct_table[x1].state,stack_struct_table[x1].stack);
    SMC_RET1(handle, 1);
}

//释放进程标识对应的内核栈基地址
uintptr_t free_pid_and_stack(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle)
{
    for(int i = stack_struct_table_size - 1; i >= 0; --i)
    {
        if(stack_struct_table[i].pid == (short)x1)
        {
            //ERROR("loc: %d, size: %d, free  pid: %d, state:%d\n", i, stack_struct_table_size, (short)stack_struct_table[i].pid, (int)stack_struct_table[i].state);
            if(i != stack_struct_table_size - 1)
            {
                //ERROR("size-1: pid: %d, state: %d, stack: 0x%016lx", (short)stack_struct_table[stack_struct_table_size - 1].pid, (int)stack_struct_table[stack_struct_table_size - 1].state, stack_struct_table[stack_struct_table_size - 1].stack);
                tzc_configure_region((uint32_t)0x0,(uint8_t)3U,stack_struct_table[i].stack,stack_struct_table[i].stack + STACK_SIZE - 1,0,0);
                stack_struct_table[i].pid = stack_struct_table[stack_struct_table_size - 1].pid;
                stack_struct_table[i].state = stack_struct_table[stack_struct_table_size - 1].state;
                stack_struct_table[i].stack = stack_struct_table[stack_struct_table_size - 1].stack;
            }
            stack_struct_table_size--;
            //visit_pid_and_stack();
            break;
        }
    }
    // ERROR("free  pid: %d, state:%d\n", (int)x1, (int)stack_struct_table[x1].state);
    SMC_RET1(handle, 1);
}

//初始化保护内核栈结构体
uintptr_t init_pid_and_stack(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle)
{
    printf("init\n\n");
    unsigned long sspool_addr = (unsigned long)SSPOOL;
    stack_struct_table = (STACK_STRUCT *) sspool_addr; 
    //ERROR("aaaaaaaa %d, 0x%016lx", stack_struct_table[0].state, stack_struct_table[0].stack);
    for(int i = 0; i < PID_SIZE; ++i)
    {
        stack_struct_table[i].pid = 0;
        stack_struct_table[i].state = 0;
        stack_struct_table[i].stack = 0;
    }
    visit_pid_and_stack();
    SMC_RET1(handle, 1);
}


//检查内核栈hash值是否与之前相同
bool check_hash(int loc, unsigned long stack_addr)
{
    char *start = (char *)stack_addr;
    SHA256_CTX ctx;
    BYTE new_hash[SHA256_BLOCK_SIZE];
    sha256_init(&ctx);
	sha256_update_stack(&ctx, (BYTE *)start, STACK_SIZE - 1);
    sha256_final(&ctx, new_hash);
    if (memcmp(stack_struct_table[loc].hash, new_hash, SHA256_BLOCK_SIZE) == 0)
        return true;
    return false;
}

// //进程切换时保存内核栈hash值
// uintptr_t set_stack_hash(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle)
// {
//     if(!check_stack((int)x1, x2))
//     {
//         ERROR("bad stack address\n");
//         SMC_RET1(handle, 0);
//     }
//     SHA256_CTX ctx;
//     BYTE new_hash[SHA256_BLOCK_SIZE];
//     sha256_init(&ctx);
// 	sha256_update_stack(&ctx, (BYTE *)stack_struct_table[x1].stack, STACK_SIZE - 1);
//     sha256_final(&ctx, new_hash);
//     memcpy(stack_struct_table[x1].hash, new_hash, SHA256_BLOCK_SIZE);
//     SMC_RET1(handle, 1);
// }


//进程切换后被切换的进程
uintptr_t switch_pid_and_stack(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle)
{
    //ERROR("Switch Start, prev pid: %d, stack:0x%016lx, next pid:%d, stack:0x%016lx\n", (short)x1, (unsigned long)x2, (short)x3, (unsigned long)x4);
    // unsigned long stack_addr = (unsigned long)x4;
    // char *start = (char *)stack_addr;
    // SHA256_CTX ctx;
    // BYTE new_hash[SHA256_BLOCK_SIZE];
    // sha256_init(&ctx);
	// sha256_update_stack(&ctx, (BYTE *)start, STACK_SIZE - 1);
    // sha256_final(&ctx, new_hash);
    // for(int i = 0; i < SHA256_BLOCK_SIZE; ++i)
    // {
    //     printf("%02x", new_hash[i]);
    // }
    // printf("\n");
    // count++;
    //int loc = 0;
    for (int i = 0; i < stack_struct_table_size; ++i)
    {
        if ((short)x3 != 0 && stack_struct_table[i].pid == (short)x3)
        {
            //loc = i;
            if (stack_struct_table[i].stack != (unsigned long)x4)
            {
                ERROR("Stack is not the origin stack, pid: %d, old:0x%016lx, new:0x%016lx\n", (short)x3, stack_struct_table[i].stack, (unsigned long)x4);
                SMC_RET1(handle, 0);
            }
        }
        else if((short)x3 != 0 && (unsigned long)x4 == stack_struct_table[i].stack)
        {
            ERROR("Stack reuse, pid in list: %d, stack:0x%016lx, next pid:%d, stack:0x%016lx\n", stack_struct_table[i].pid, stack_struct_table[i].stack, (short)x3, (unsigned long)x4);
            SMC_RET1(handle, 0);
        }
    }
    tzc_configure_region((uint32_t)0x1,(uint8_t)3U,x2,x2 + STACK_SIZE - 1,TZC_REGION_S_RDWR,0x8303);
    //test
    //tzc_configure_region((uint32_t)0x1,(uint8_t)3U,x4,x4 + STACK_SIZE - 1,TZC_REGION_S_RDWR,0x8303);
    tzc_configure_region((uint32_t)0x0,(uint8_t)3U,x4,x4,0,0);
    // if(count == 30)
    //     panic();
    // if((short)x3 != 0 && !check_hash(loc, (unsigned long)x4))
    // {
    //     // char *old = (char *)stack_struct_table[loc].stack;
    //     // char *new = (char *)x4;
    //     // for(int i = 15360; i < 16384; ++i)
    //     // {
    //     //     ERROR("old:%d:0x%016lx new:0x%016lx\n", i, (unsigned long)(*old), (unsigned long)(*new));
    //     //     old++;
    //     //     new++;
    //     // }
    //     // panic();
    //     ERROR("Stack is modified, prev pid: %d, stack:0x%016lx, next pid:%d, stack:0x%016lx", (short)x1, (unsigned long)x2, (short)x3, (unsigned long)x4);
    //     SMC_RET1(handle, 0);
    // }
    // for (int i = 0; i < stack_struct_table_size; ++i)
    // {
    //     if (stack_struct_table[i].pid == (short)x1)
    //     {
    //         loc = i;
    //         break;
    //     }
    // }
    // SHA256_CTX ctx;
    // BYTE new_hash[SHA256_BLOCK_SIZE];
    // sha256_init(&ctx);
	// sha256_update_stack(&ctx, (BYTE *)stack_struct_table[loc].stack, STACK_SIZE - 1);
    // sha256_final(&ctx, new_hash);
    // memcpy(stack_struct_table[loc].hash, new_hash, SHA256_BLOCK_SIZE);
    // ERROR("Switch Complete");
    SMC_RET1(handle, 1);
}

// //检查内核栈基地址和hash值是否与存储的相同
// uintptr_t check_pid_and_stack(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle)
// {
//     if(!check_stack((int)x1, x2))
//     {
//         ERROR("bad stack address\n");
//         SMC_RET1(handle, 0);
//     }
//     if(!check_hash((int)x1, x2))
//     {
//         ERROR("bad hash\n");
//         SMC_RET1(handle, 0);
//     }
//     SMC_RET1(handle, 1);
// }


//设置进程凭证
uintptr_t set_cred(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle)
{
    for(int i = 0; i < cred_struct_table_size; ++i)
    {
        if(cred_struct_table[i].pid == (short)x1)
        {
            cred_struct_table[cred_struct_table_size].task_struct_addr = (unsigned long)x2;
            cred_struct_table[cred_struct_table_size].cred_addr = (unsigned long)x3;
            cred_struct_table[cred_struct_table_size].uid = *((unsigned int *)((unsigned long)x3 + 4));
            cred_struct_table[cred_struct_table_size].gid = *((unsigned int *)((unsigned long)x3 + 8));
            cred_struct_table[cred_struct_table_size].suid = *((unsigned int *)((unsigned long)x3 + 12));
            cred_struct_table[cred_struct_table_size].sgid = *((unsigned int *)((unsigned long)x3 + 16));
            cred_struct_table[cred_struct_table_size].euid = *((unsigned int *)((unsigned long)x3 + 20));
            cred_struct_table[cred_struct_table_size].egid = *((unsigned int *)((unsigned long)x3 + 24));
            cred_struct_table[cred_struct_table_size].pgd = (unsigned long)x4;
            SMC_RET1(handle, 1);
        }
    }
    if (cred_struct_table_size >= PID_SIZE)
    {
        ERROR("cred_struct_table_size >= PID_SIZE");
        panic();
    }
    cred_struct_table[cred_struct_table_size].pid = (short)x1;
    cred_struct_table[cred_struct_table_size].task_struct_addr = (unsigned long)x2;
    cred_struct_table[cred_struct_table_size].cred_addr = (unsigned long)x3;
    // ERROR("pid:%d, task_addr: %016lx, cred_addr:%016lx", cred_struct_table[cred_struct_table_size].pid, cred_struct_table[cred_struct_table_size].task_struct_addr, cred_struct_table[cred_struct_table_size].cred_addr);
    // ERROR("0x%016lx", (unsigned long)x3 + 4);
    // unsigned int *a = (unsigned int *)((unsigned long)x3 + 4);
    // short *a = (short *)((unsigned long)x2 + 1120);
    // ERROR("pid: %d ", *a);
    // ERROR("pid: %u, addr:0x%016lx", *a, (unsigned long)a);
    cred_struct_table[cred_struct_table_size].uid = *((unsigned int *)((unsigned long)x3 + 4));
    cred_struct_table[cred_struct_table_size].gid = *((unsigned int *)((unsigned long)x3 + 8));
    cred_struct_table[cred_struct_table_size].suid = *((unsigned int *)((unsigned long)x3 + 12));
    cred_struct_table[cred_struct_table_size].sgid = *((unsigned int *)((unsigned long)x3 + 16));
    cred_struct_table[cred_struct_table_size].euid = *((unsigned int *)((unsigned long)x3 + 20));
    cred_struct_table[cred_struct_table_size].egid = *((unsigned int *)((unsigned long)x3 + 24));
    cred_struct_table[cred_struct_table_size].pgd = (unsigned long)x4;

    ERROR("pid:%d, cred: 0x%016lx, uid:%u, gid:%u, suid:%u, sgid:%u, euid:%u, egid:%u\n", cred_struct_table[cred_struct_table_size].pid, cred_struct_table[cred_struct_table_size].cred_addr, cred_struct_table[cred_struct_table_size].uid, cred_struct_table[cred_struct_table_size].gid, cred_struct_table[cred_struct_table_size].suid, cred_struct_table[cred_struct_table_size].sgid, cred_struct_table[cred_struct_table_size].euid, cred_struct_table[cred_struct_table_size].egid);
    cred_struct_table_size++;
    
    SMC_RET1(handle, 1);
}

//检查进程凭证
uintptr_t check_cred(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle)
{
    unsigned int uid =  *((unsigned int *)((unsigned long)x3 + 4));
    unsigned int gid =  *((unsigned int *)((unsigned long)x3 + 8));
    unsigned int suid =  *((unsigned int *)((unsigned long)x3 + 12));
    unsigned int sgid =  *((unsigned int *)((unsigned long)x3 + 16));
    unsigned int euid =  *((unsigned int *)((unsigned long)x3 + 20));
    unsigned int egid =  *((unsigned int *)((unsigned long)x3 + 24));
    for (int i = 0; i < cred_struct_table_size; ++i)
    {
        if (cred_struct_table[i].pid == (short)x1)
        {
            if (cred_struct_table[i].cred_addr != (unsigned long)x3)
            {
                ERROR("cred_addr is not origin");
                panic();
            }
            else if (cred_struct_table[i].task_struct_addr != (unsigned long)x2)
            {
                ERROR("task_struct_addr is not origin");
                panic();
            }
            else if (cred_struct_table[i].pgd != (unsigned long)x4)
            {
                ERROR("pgd is not origin");
                panic();
            }
            else if (cred_struct_table[i].uid != uid)
            {
                ERROR("uid is not origin");
                panic();
            }
            else if (cred_struct_table[i].gid != gid)
            {
                ERROR("gid is not origin");
                panic();
            }
            else if (cred_struct_table[i].suid != suid)
            {
                ERROR("suid is not origin");
                panic();
            }
            else if (cred_struct_table[i].sgid != sgid)
            {
                ERROR("sgid is not origin");
                panic();
            }
            else if (cred_struct_table[i].euid != euid)
            {
                ERROR("euid is not origin");
                panic();
            }
            else if (cred_struct_table[i].egid != egid)
            {
                ERROR("egid is not origin");
                panic();
            }
        }
    }
    SMC_RET1(handle, 1);
}

//释放进程凭证
uintptr_t free_cred(u_register_t x1,u_register_t x2,u_register_t x3,u_register_t x4,void *handle)
{
    for(int i = cred_struct_table_size - 1; i >= 0; --i)
    {
        if(cred_struct_table[i].pid == (short)x1)
        {
            ERROR("free_cred size: %d, free  pid: %d\n", cred_struct_table_size, (short)cred_struct_table[i].pid);
            if(i != cred_struct_table_size - 1)
            {
                cred_struct_table[i].pid = cred_struct_table[cred_struct_table_size - 1].pid;
                cred_struct_table[i].task_struct_addr = cred_struct_table[cred_struct_table_size - 1].task_struct_addr;
                cred_struct_table[i].cred_addr = cred_struct_table[cred_struct_table_size - 1].cred_addr;
                cred_struct_table[i].pgd = cred_struct_table[cred_struct_table_size - 1].pgd;
                cred_struct_table[i].uid = cred_struct_table[cred_struct_table_size - 1].uid;
                cred_struct_table[i].gid = cred_struct_table[cred_struct_table_size - 1].gid;
                cred_struct_table[i].suid = cred_struct_table[cred_struct_table_size - 1].suid;
                cred_struct_table[i].sgid = cred_struct_table[cred_struct_table_size - 1].sgid;
                cred_struct_table[i].euid = cred_struct_table[cred_struct_table_size - 1].euid;
                cred_struct_table[i].egid = cred_struct_table[cred_struct_table_size - 1].egid;
            }
            cred_struct_table_size--;
            break;
        }
    }
    SMC_RET1(handle, 1);
}