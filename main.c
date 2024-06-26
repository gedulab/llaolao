/*
 This is an example of a minimal character device driver by Raymond Zhang with 
 contributions from his colleagues in GEDU.
  
 Usage examples:	
 kernel stack overflow:
 echo 2000 > /sys/kernel/debug/llaolao/age
 
 Porbe
 echo probe > /proc/llaolao
*/
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/dcache.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <asm/mmu_context.h>
#include <asm/tlbflush.h>
#include <asm/io.h>
#include "gearm.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
#define HAVE_PROC_OPS
#endif

#define DEVICE_NAME      "huadeng2"
#define MAJOR_NUM        88
#define NUM_DEVICES      6
#define DEV_DATA_LENGTH  100
#define TARGET_NAME_MAX  16

static struct class *huadeng_class;
struct huadeng_dev {
  unsigned short current_pointer; /* Current pointer within the
                                     device data region */
  unsigned int size;              /* Size of the bank */
  int number;                     /* device number */
  struct cdev cdev;               /* The cdev structure */
  char name[10];                  /* Name of the device */
  char data[DEV_DATA_LENGTH];
  /* ... */                       /* Mutexes, spinlocks, wait
                                     queues, .. */
} *hd_devp[NUM_DEVICES];

struct lll_profile_struct {
    int             age;
    struct file *   file;
    // other stuffs
};
static char* hostip = "192.168.0.101";
static char* targetip = "192.168.8.1";
// static char targetname[TARGET_NAME_MAX];
static unsigned short hostport = 5000;
static unsigned short targetport = 6000;
static struct proc_dir_entry *proc_lll_entry = NULL ;
int g_seed = 0;
extern gd_box g_box; // attributes of hardware running on 

// static varibales for debugfs
static struct dentry *df_dir = NULL, * df_dir;

static void wastestack(int recursive)
{
	printk("wastestack %d\n",recursive);

	while(recursive--)
		wastestack(recursive);
}

long ge_probe_kernel_read(void *dst, const void *src, size_t size)
{
	long ret = 0;
#ifdef TRY_FS	
	mm_segment_t old_fs = get_fs();

	set_fs(KERNEL_DS);
	pagefault_disable();
	ret = __copy_from_user_inatomic(dst,
			(__force const void __user *)src, size);
	pagefault_enable();
	set_fs(old_fs);
#endif // TRY_FS
	return ret ? -EFAULT : 0;
}

static void	ge_probe(void)
{
	char data[32] = {0};
	long addr = 0x880;

    long ret = ge_probe_kernel_read(data, (const void *)addr, sizeof(data));
	printk("probe %lx got %ld\n", addr, ret);
}

static int
lll_age_set(void *data, u64 val)
{
    struct lll_profile_struct * lp = (struct lll_profile_struct *)data;
    lp->age = val;
	if(val==200)
		wastestack(val);

    return 0;
}
static int
lll_age_get(void *data, u64 *val)
{
    struct lll_profile_struct * lp = (struct lll_profile_struct *)data;
    *val = lp->age;

    return 0;
}

// macro from linux/fs.h
DEFINE_SIMPLE_ATTRIBUTE(df_age_fops, lll_age_get, lll_age_set, "%llu\n");

static ssize_t proc_lll_read(struct file *filp, char __user * buf, size_t count, loff_t * offp)
{
    int n = 0, ret;
    char secrets[100];

    printk(KERN_INFO "proc_lll_read is called file %p, buf %p count %ld off %llx\n",
        filp, buf, count, *offp);
    sprintf(secrets, "kernel secrets balabala %s...\n", filp->f_path.dentry->d_iname);	

    n = strlen(secrets);
    if(*offp < n)
    {
	ret = copy_to_user(buf, secrets, n+1);
        *offp = n+1;
        ret = n+1;
    }
    else
        ret = 0;

    return ret; 
}
/* A timer list */
struct timer_list kt_timer;
 
/* Timer callback */
void ll_timer_callback(unsigned long arg)
{
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);
 
	*(int*)arg = 0xbad0bad0;
    //mod_timer(&kt_timer, jiffies + 10*HZ); /* restarting timer */
}

int ll_writenull_in_timer(void)
{
/*	init_timer(&kt_timer);
	kt_timer.function = ll_timer_callback;
	kt_timer.data = (unsigned long)0xbad;
	kt_timer.expires = jiffies + 5*HZ; // 5 second 

	add_timer(&kt_timer); // Starting the timer 
    */
	printk(KERN_INFO "readnull_timer is started\n");
	return 0;
}

static void ll_print_pgtable_macro(void)
{
    printk("PAGE_OFFSET = 0x%lx\n", PAGE_OFFSET);
    printk("PGDIR_SHIFT = %d\n", PGDIR_SHIFT);
    printk("PUD_SHIFT = %d\n", PUD_SHIFT);
    printk("PMD_SHIFT = %d\n", PMD_SHIFT);
    printk("PAGE_SHIFT = %d\n", PAGE_SHIFT);

    printk("PTRS_PER_PGD = %d\n", PTRS_PER_PGD);
    printk("PTRS_PER_PUD = %d\n", PTRS_PER_PUD);
    printk("PTRS_PER_PMD = %d\n", PTRS_PER_PMD);
    printk("PTRS_PER_PTE = %d\n", PTRS_PER_PTE);

    printk("PAGE_MASK = 0x%lx\n", PAGE_MASK);
}

static unsigned long ll_v2p(unsigned long vaddr)
{
    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    unsigned long paddr = 0;
        unsigned long page_addr = 0;
    unsigned long page_offset = 0;

    pgd = pgd_offset(current->mm, vaddr);
    printk("pgd_val = 0x%llx\n", pgd_val(*pgd));
    printk("pgd_index = %lu\n", pgd_index(vaddr));
    if (pgd_none(*pgd)) {
        printk("not mapped in pgd\n");
        return -1;
    }

    pud = pud_offset((p4d_t*)pgd, vaddr);
    printk("pud_val = 0x%llx\n", pud_val(*pud));
    if (pud_none(*pud)) {
        printk("not mapped in pud\n");
        return -1;
    }

    pmd = pmd_offset(pud, vaddr);
    printk("pmd_val = 0x%llx\n", pmd_val(*pmd));
    printk("pmd_index = %lu\n", pmd_index(vaddr));
    if (pmd_none(*pmd)) {
        printk("not mapped in pmd\n");
        return -1;
    }

    pte = pte_offset_kernel(pmd, vaddr);
    printk("pte_val = 0x%llx\n", pte_val(*pte));
    printk("pte_index = %lu\n", pte_index(vaddr));
    if (pte_none(*pte)) {
        printk("not mapped in pte\n");
        return -1;
    }

    /* Page frame physical address mechanism | offset */
    page_addr = pte_val(*pte) & PAGE_MASK;
    page_offset = vaddr & ~PAGE_MASK;
    paddr = page_addr | page_offset;
    printk("page_addr = %lx, page_offset = %lx\n", page_addr, page_offset);
        printk("vaddr = %lx, paddr = %lx\n", vaddr, paddr);

    return paddr;
}

static DEFINE_PER_CPU(unsigned long, ge_counter);
void ge_percpu(void)
{
    unsigned long p0, p1,p2,p3;
    unsigned long old = __this_cpu_read(ge_counter);
    __this_cpu_write(ge_counter, old+1);
    // printk("per_cpu_start %lx, per_cpu_end %lx current %lx\n",
        // (unsigned long)__per_cpu_start, (unsigned long)__per_cpu_end, (unsigned long)current);

    p0 = (unsigned long)per_cpu_ptr(&ge_counter, 0);
    p1 = (unsigned long)per_cpu_ptr(&ge_counter, 1);
    p2 = (unsigned long)per_cpu_ptr(&ge_counter, 2);
    p3 = (unsigned long)per_cpu_ptr(&ge_counter, 3);

    printk("percpu var at %lx, %lx, %lx %lx\n", p0, p1, p2, p3);
    p0 = (unsigned long)per_cpu_ptr(current, 0);
    p1 = (unsigned long)per_cpu_ptr(current, 1);
    p2 = (unsigned long)per_cpu_ptr(current, 2);
    p3 = (unsigned long)per_cpu_ptr(current, 3);

    printk("percpu current at %lx, %lx, %lx %lx\n", p0, p1, p2, p3);
    //p3 = (unsigned long)per_cpu_ptr(&current, 3);
}
struct ndb_geo_block {
    unsigned int magic_;
    unsigned short ver_major_;
    unsigned short ver_minor_;
    // other fields
};

#define YL_IRAM_BASE 0xff090000
#define YL_IRAM_SIZE 0x9000
#define YL_IRAM_ADDR 0xff098010
#define GE_MISALIGN_IOREMAP_DDR  1
#define GE_MISALIGN_IOREMAP_IRAM 2
#define GE_MISALIGN_FIXMAP_IO    3
#define GE_MISALIGN_FIXMAP       4

static const char*  ge_misalign_option_name(int option)
{
   switch(option) {
      case GE_MISALIGN_IOREMAP_DDR:// 1
	return "ioremap_ddr";
      case GE_MISALIGN_IOREMAP_IRAM:// 2
	return "ioremap_iram";
      case GE_MISALIGN_FIXMAP_IO://    3
	return "fixmap_io";
      case GE_MISALIGN_FIXMAP:// 4
        return "fixmap";
      default:
	return "unknown";
   }
}

int ge_mem_misalign(int option)
{
    long    pfn = 0;
    void * ptr;    
    struct ndb_geo_block* ngb;
    if (option == GE_MISALIGN_IOREMAP_DDR) {
        struct page* pg;    
        pg = alloc_pages_node(numa_node_id(), __GFP_HIGHMEM | __GFP_ZERO, 0);
        if (!pg)
        {
            printk("alloc page failed\n");
            return -ENOMEM;
        }
    
        pfn = page_to_pfn(pg);
        ptr = ioremap(pfn<<12, 4096);
    }
    else if (option == GE_MISALIGN_IOREMAP_IRAM) {
        ptr = ioremap(YL_IRAM_BASE, YL_IRAM_SIZE);
    } 
    else {
        void __iomem* base;
	if (option == GE_MISALIGN_FIXMAP) 
	    set_fixmap(FIX_EARLYCON_MEM_BASE, YL_IRAM_ADDR & PAGE_MASK);
	else
            set_fixmap_io(FIX_EARLYCON_MEM_BASE, YL_IRAM_ADDR & PAGE_MASK);
        base = (void __iomem *)__fix_to_virt(FIX_EARLYCON_MEM_BASE);
        base += YL_IRAM_ADDR & ~PAGE_MASK;
        if(!base) {
	    printk(KERN_ERR "failed to map IRAM at 0x%x\n", YL_IRAM_ADDR);
	    return -ENXIO;
        }
        ptr = base + (YL_IRAM_ADDR & ~PAGE_MASK);
    }    
    if(!ptr) {
	printk("ioremap(%lx) failed\n", pfn);
	return -ENXIO;
    }
    printk("ngb is at %pK using %s\n", ptr, ge_misalign_option_name(option));
    ngb = (struct ndb_geo_block*)ptr;
    ngb->magic_ = 0x47454455; //'GEDU';
    // ngb->ver_major_ = 1;
    //ngb->ver_minor_ = 10;

    //
    *(int*)&ngb->ver_minor_ = 10; 

    return 0;
    
}
int ge_dump_mmio(u64 addr, u32 size)
{
    u32 value, i; 
    void* base = ioremap(addr, size);
    if (base == NULL) {
        printk(KERN_ERR "failed to map MMIO at %llx size %x\n", addr, size);
        return -1;
    }
    printk("MMIO dump from 0x%llx, size 0x%x\n", addr, size);
    for (i = 0; i < size; i+=4){
        value = readl(base + i);
        printk("  0x%llx: %0x\n", addr + i, value);
    }
    iounmap(base);

    return 0;
}
#define LLL_MAX_PARAS 5
static ssize_t proc_lll_write(struct file *file, const char __user *buffer,
			 size_t count, loff_t *data)
{
    char cmd[100]={0x00}, *para;
    unsigned long addr = 0, para_longs[LLL_MAX_PARAS] = {0};
    int para_count = 0;

    printk("proc_lll_write called legnth 0x%lx, %p\n", count, buffer);
    if (count < 1)
    {
       printk("count <=1\n");
       return -EBADMSG; /* runt */
    }
    if (count > sizeof(cmd))
    {
        printk("count > sizeof(cmd)\n");
        return -EFBIG;   /* too long */
    }

    if (copy_from_user(cmd, buffer, count))
        return -EFAULT;
    para = cmd;    
    do {
        para = strchr(para, ' ');
        if (para) {
            *para = 0;
            para++;
            para_longs[para_count] = simple_strtoul(para, NULL, 0);
            para_count++;
        }
    } while(para != NULL);

    if(strncmp(cmd,"div0",4) == 0)
    {
	printk("going to divide %d/%ld\n", g_seed, count-5);

        g_seed = g_seed/(count-5); 
    }
    else if(strncmp(cmd, "nullp", 5) == 0)
    {
        *(int*)(long)0x880 = 0x88888888;
    }
    else if(strncmp(cmd, "probe", 5) == 0)
    {
		ge_probe();
    }
    else if(strncmp(cmd,"timer0", 6) == 0)
    {
	    ll_writenull_in_timer();
    }
    else if(strncmp(cmd, "pte", 3) == 0)
    {
        addr = para_longs[0];
        addr = (addr == 0) ? ((unsigned long)buffer) : addr;
	    printk("physical address of %lx = %lx\n", addr, ll_v2p(addr));
        ll_print_pgtable_macro();
    }
    else if (strncmp(cmd, "percpu", 6) == 0) {
        ge_percpu();
    }
#ifdef CONFIG_ARM64    
    else if(strncmp(cmd, "sysreg", 6) == 0)
    {
	    ge_arm_sysregs();
    }
    else if (strncmp(cmd, "jtag", 4) == 0)
    {
        ge_arm_switch_jtag(1);
        ge_arm_enable_jtag_clk(1);
    }
    else if (strncmp(cmd, "jtagoff", 7) == 0)
    {
        ge_arm_switch_jtag(0);
        ge_arm_enable_jtag_clk(0);
    }
    else if (strncmp(cmd, "hot", 3) == 0)
    {
        ge_arm_read_tsadc(&g_box,0);
        ge_arm_read_tsadc(&g_box,1);
    }
    else if (strncmp(cmd, "iram", 4) == 0) {
        ge_iram((int)para_longs[0], para_longs[1]);
    }
    else if (strncmp(cmd, "ulan", 4) == 0) {
        ge_yl1_switch_jtag(1);
    }
    else if (strncmp(cmd, "3588_uart_on", 12) == 0) {
    	ge_yl1_switch_uart(1);
    }
    else if (strncmp(cmd, "3588_sdsts", 10) == 0) {
    	ge_yl1_switch_sd_status_check();
    }
#endif // CONFIG_ARM64    
    else if (strncmp(cmd, "mmio", 4) == 0) {
        if(para_count > 1) {
    	    ge_dump_mmio(para_longs[0], para_longs[1]);
        } else {
            printk("missing arguments, pls. try mmio <base> <size>\n");
        }
    }
    else if (strncmp(cmd, "align", 5) == 0) {
    	ge_mem_misalign((int)para_longs[0]);
    }
    else if(strncmp(cmd, "hlt", 3) == 0) {
	ge_hlt((int)para_longs[0]);
    }
    else
    {
        printk("unsupported cmd '%s'\n", cmd);
    }

    return count;	
}

#ifdef HAVE_PROC_OPS
static const struct proc_ops proc_lll_fops = {
 .proc_read = proc_lll_read,
 .proc_write = proc_lll_write,
};
#else
static const struct file_operations proc_lll_fops = {
 .owner = THIS_MODULE,
 .read = proc_lll_read,
 .write = proc_lll_write,
};
#endif

static int huadeng_open(struct inode *inode, struct file *file)
{
  struct huadeng_dev *hd_devp;
  pr_info("%s\n", __func__);

  /* Get the per-device structure that contains this cdev */
  hd_devp = container_of(inode->i_cdev, struct huadeng_dev, cdev);

  /* Easy access to cmos_devp from rest of the entry points */
  file->private_data = hd_devp;

  /* Initialize some fields */
  hd_devp->size = DEV_DATA_LENGTH;
  hd_devp->current_pointer = 0;

  return 0;
}

static int huadeng_release(struct inode *inode, struct file *file)
{
  struct huadeng_dev *hd_devp = file->private_data;
  pr_info("%s\n", __func__);

  /* Reset file pointer */
  hd_devp->current_pointer = 0;

  return 0;
}
int g_vfile_pecent = -1;

static ssize_t huadeng_read(struct file *file,
  char *buffer, size_t count, loff_t * offset)
{
  struct huadeng_dev *hd_devp = file->private_data;
  pr_info("%s %lu, +%llx\n", __func__, count, *offset);

  if (*offset >= hd_devp->size) {
    return 0; /*EOF*/
  }
  /* Adjust count if its edges past the end of the data region */
  if (*offset + count > hd_devp->size) {
    count = hd_devp->size - *offset;
  }
  /* Copy the read bits to the user buffer */
  if (copy_to_user(buffer, (void *)(hd_devp->data + *offset), count) != 0) {
    return -EIO;
  }
    
  return count;
}

static ssize_t huadeng_write(struct file *file,
  const char *buffer, size_t length, loff_t * offset)
{
	int i;
	unsigned long percent;

	struct huadeng_dev *hd_devp = file->private_data;
	pr_info("%s %lu +%llx\n", __func__, length, *offset);
	if(*offset >= hd_devp->size) {
		return 0;
	}  
	if (*offset + length > hd_devp->size) {
		length = hd_devp->size - *offset;
	}
	if(copy_from_user(hd_devp->data+*offset, buffer, length) !=0 ) {
		printk(KERN_ERR "copy_from_user failed\n");
		return -EFAULT;
	} 
	percent = simple_strtoul(hd_devp->data, NULL, 0x10);

	printk("v_store: %s %ld\n", hd_devp->data, length);
	for (i = 0; i < length; i++)
		printk(" %c ", hd_devp->data[i]);
	printk("\n");

	if (percent > 100)
		g_vfile_pecent = 100;
	else
		g_vfile_pecent = percent;

	//
	//	update the data buffer in dev's private data structure to ease geduers
	sprintf(hd_devp->data, "%d\%%\n", g_vfile_pecent);
	//
 
	return length;
}

struct file_operations huadeng_fops = {
  .owner = THIS_MODULE,
  .open = huadeng_open,
  .release = huadeng_release,
  .read = huadeng_read,
  .write = huadeng_write,
};
static bool slowinit;
module_param(slowinit, bool, 0444);
MODULE_PARM_DESC(slowinit, "do a delay in module init to wait for debugger");

static int __init llaolao_init(void)
{
    int n = 0x1937, ret = 0, i;
    static struct lll_profile_struct lll_profile;
    enum gd_box_id box;
    printk(KERN_INFO "Hi, I am llaolao at address: %p\n symbol: 0x%pF\n stack: 0x%p\n"
        " first 16 bytes: %*phC\n",
            llaolao_init, llaolao_init, &n, 16, (char*)llaolao_init);
    if (slowinit){
          printk("busy waiting for a debugger...\n");
          mdelay(10000L);
    }
    
#ifdef CONFIG_ARM64
    box = get_gd_box_version(get_gd_box_id());
    if(box != GD_BOX_UNKNOWN) {
       printk("Running on GEDU Box=%s", box == GD_BOX_GDK8 ?"GDK8":"ULAN");
       gd_init_box(&g_box, box);
    }
#endif 

#ifdef CHRDRV_OLD_STYLE
    ret = register_chrdev(MAJOR_NUM, DEVICE_NAME, &huadeng_fops);
    if (ret != 0) {
        printk(KERN_ERR "register_chrdev failed %d\n", ret);
        return ret;
    }
#endif

    huadeng_class = class_create(THIS_MODULE, DEVICE_NAME);
    for (i = 0; i < NUM_DEVICES; i++) {
        /* Allocate memory for the per-device structure */
        hd_devp[i] = kmalloc(sizeof(struct huadeng_dev), GFP_KERNEL);
        if (!hd_devp[i]) {
            printk("allocate huadeng dev struct failed\n"); 
            return -ENOMEM;
        }
        /* Connect the file operations with the cdev */
        cdev_init(&hd_devp[i]->cdev, &huadeng_fops);
        hd_devp[i]->cdev.owner = THIS_MODULE;

        /* Connect the major/minor number to the cdev */
        ret = cdev_add(&hd_devp[i]->cdev, MKDEV(MAJOR_NUM, i), 1);
        if (ret) {
          printk(KERN_ERR "cdev_add failed for %d\n", i);
          return ret;
        }

        /* Send uevents to udev, so it'll create /dev nodes */
        device_create(huadeng_class, NULL,
            MKDEV(MAJOR_NUM, i), NULL, "huadeng%d", i);
    }

    /* Create /proc/llaolao */
    proc_lll_entry = proc_create("llaolao", 0, NULL, &proc_lll_fops);
    if (proc_lll_entry == NULL) {
        printk(KERN_ERR "create proc entry failed\n");
        return -1;
    }
    df_dir = debugfs_create_dir("llaolao", 0);
    if(!df_dir)
    {
        printk(KERN_ERR "create dir under debugfs failed\n");
        return -1;
    }

    debugfs_create_file("age", 0222, df_dir, &lll_profile, &df_age_fops);

    return 0;
}

static void __exit llaolao_exit(void)
{
    int i;
    printk("Exiting from 0x%p... Bye, GEDU friends\n", llaolao_exit);
    if(proc_lll_entry)
        proc_remove(proc_lll_entry);
    if(df_dir)
      // clean up all debugfs entries
      debugfs_remove_recursive(df_dir);
    if(huadeng_class) {
        for (i = 0; i < NUM_DEVICES; i++) {
            device_destroy(huadeng_class, MKDEV(MAJOR_NUM, i));
            cdev_del(&hd_devp[i]->cdev);
            kfree(hd_devp[i]);
        }
        class_destroy(huadeng_class);
#ifdef CHRDRV_OLD_STYLE
        unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
#endif
    }

    printk(KERN_EMERG "Testing message with KERN_EMERG severity level\n");
    printk(KERN_ALERT "Testing message with KERN_ALERT severity level\n");
    printk(KERN_CRIT "Testing message with KERN_CRIT severity level\n");
    printk(KERN_ERR "Testing message with KERN_ERR severity level\n");
    printk(KERN_WARNING "Testing message with KERN_WARNING severity level\n");
    printk(KERN_NOTICE "Testing message with KERN_NOTICE severity level\n");
    printk(KERN_INFO "Testing message with KERN_INFO severity level\n");
    printk(KERN_DEBUG "Testing message with KERN_DEBUG severity level\n");
}

module_init(llaolao_init);
module_exit(llaolao_exit);
module_param(hostip, charp, S_IRUGO);
module_param(targetip, charp, S_IRUGO);
module_param(hostport, ushort, S_IRUGO);
module_param(targetport, ushort, S_IRUGO);

MODULE_AUTHOR("GEDU lab");

MODULE_DESCRIPTION("LKM example - llaolao");
MODULE_LICENSE("GPL");
