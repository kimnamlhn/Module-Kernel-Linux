#include <asm/unistd.h>
#include <asm/cacheflush.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <asm/pgtable_types.h>
#include <linux/highmem.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/moduleparam.h>
#include <linux/unistd.h>
#include <asm/cacheflush.h>
MODULE_LICENSE("GPL");
MODULE_AUTHOR("TanKiet");
/*MY sys_call_table address*/
//ffffffff820001a0
void **system_call_table_addr;
/*my custom syscall that takes process name*/
asmlinkage int (*custom_syscall)(int fd, const void *buf, size_t count);
/*hook*/
// Hook function, replace the system call WRITE
// Take the same parameter as the system call WRITE
ssize_t hook_function(int fd, const void *buf, size_t count)
{
    // Print calling process name
    printk(KERN_INFO "Calling process:%s\n", current->comm);
    int written_bytes = custom_syscall(fd, buf, count);
    printk(KERN_INFO "Number of written bytes:%d\n", written_bytes);
    return written_bytes;
}
/*Make page writeable*/
int make_rw(unsigned long address)
{
    unsigned int level;
    pte_t *pte = lookup_address(address, &level);
    if (pte->pte & ~_PAGE_RW)
    {
        pte->pte |= _PAGE_RW;
    }
    return 0;
}
/* Make the page write protected */
int make_ro(unsigned long address)
{
    unsigned int level;
    pte_t *pte = lookup_address(address, &level);
    pte->pte = pte->pte & ~_PAGE_RW;
    return 0;
}
static int __init entry_point(void)
{
    //system call table address
    system_call_table_addr = (void *)0xffffffff820001a0;
    // Assign custom_syscall to system call OPEN
    custom_syscall = system_call_table_addr[__NR_write];
    //Disable page protection
    make_rw((unsigned long)system_call_table_addr);
    // Replace system call OPEN by our Hook function
    system_call_table_addr[__NR_write] = hook_function;
    return 0;
}
static void __exit exit_point(void)
{
    printk(KERN_INFO "Unloaded Captain Hook successfully\n");
    /*Restore original system call */
    system_call_table_addr[__NR_write] = custom_syscall;
    /*Renable page protection*/
    make_ro((unsigned long)system_call_table_addr);
    return;
}
module_init(entry_point);
module_exit(exit_point);