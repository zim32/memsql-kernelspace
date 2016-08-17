#include <linux/init.h>
#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include "../include/types.h"
#include <linux/irqflags.h>
#include <linux/slab.h>
#include <linux/dcache.h>
#include <linux/stop_machine.h>
#include <linux/hugetlb_inline.h>
#include <asm/pgtable_types.h>

#define ALLOCATED_PAGES_NUM 15000
#define GLOBAL_BUFFER_SIZE (ALLOCATED_PAGES_NUM*PAGE_SIZE)

#include "./pagewalk.c"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zim32");
MODULE_DESCRIPTION("A Simple Hello World module");


static char *global_buf = 0;
static size_t total_dumped = 0;


static void fill_with_zeroes(char *buf, size_t size, size_t *transfered){
	memset(buf, 0, size);
	*transfered += size;
}


static bool dump_processes(char *buf, size_t *transfered){
	struct task_struct *task;
	zim32_task_t ztask;
	size_t ztask_size = sizeof(zim32_task_t);
	unsigned counter = 0;

	printk("Task structure size: %zu\n", ztask_size);

	for_each_process(task) {
		if(*transfered + ztask_size >= GLOBAL_BUFFER_SIZE){
			printk("Buffer overflow");
			return false;
		}
		counter++;

		// reset structures
		memset(&ztask, 0, ztask_size);

		// fill data
		ztask.id = task;
		strcpy(ztask.cmd, task->comm);
		if(task->mm){
			ztask.mm_id = task->mm;
		}

		memcpy(buf+*transfered, &ztask, ztask_size);
		*transfered += ztask_size;
	}

	// make zeroed space
	fill_with_zeroes(buf+*transfered, ztask_size, transfered);

	printk("Total tasks structures: %u\n", counter);
	return true;
}


static bool dump_mm(char *buf, size_t *transfered){
	struct task_struct *task;
	zim32_mm_t zmm;
	size_t zmm_size = sizeof(zim32_mm_t);
	pgd_t pgd;
	unsigned counter = 0;

	printk("mm structure size: %zu\n", zmm_size);

	for_each_process(task) {
		// fill data
		if(!task->mm){
			continue;
		}

		if(*transfered + zmm_size >= GLOBAL_BUFFER_SIZE){
			printk("Buffer overflow");
			return false;
		}
		counter++;

		// reset structures
		memset(&zmm, 0, zmm_size);

		pgd = *(task->mm->pgd);
		zmm.pgd.data = pgd_val(pgd);
		zmm.id = task->mm;
		zmm.task_id = task;
		zmm.start_code = task->mm->start_code;
		zmm.end_code = task->mm->end_code;
		zmm.start_data = task->mm->start_data;
		zmm.end_data = task->mm->end_data;
		zmm.start_brk = task->mm->start_brk;
		zmm.brk = task->mm->brk;
		zmm.start_stack = task->mm->start_stack;

		memcpy(buf+*transfered, &zmm, zmm_size);
		*transfered += zmm_size;
	}

	// make zeroed space
	fill_with_zeroes(buf+*transfered, zmm_size, transfered);

	printk("Total mm structures: %u\n", counter);

	return true;
}


static bool dump_vm_areas(char *buf, size_t *transfered){
	struct task_struct *task;
	zim32_vm_area_t *zvmarea;
	void *vm_area_id = 0;
	struct vm_area_struct *vmarea;
	size_t zvmarea_size = sizeof(zim32_vm_area_t);
	unsigned counter = 0;
	char *file_name_buf;
	char *returned_buf_pointer = 0;

	file_name_buf = (char *)kmalloc(1024, GFP_ATOMIC);
	zvmarea = (zim32_vm_area_t *)kmalloc(zvmarea_size, GFP_ATOMIC);

	printk("vmarea structure size: %zu\n", zvmarea_size);

	for_each_process(task) {

		if(!task->mm || !task->mm->mmap){
			continue;
		}

		vmarea = task->mm->mmap;
		vm_area_id = task->mm->mmap;

		do{
			if(*transfered + zvmarea_size >= GLOBAL_BUFFER_SIZE){
				printk("Buffer overflow");
				return false;
			}
			counter++;

			// reset structures
			memset(zvmarea, 0, zvmarea_size);

			zvmarea->id = vm_area_id;
			zvmarea->mm_id = vmarea->vm_mm;
			zvmarea->vm_start = vmarea->vm_start;
			zvmarea->vm_end = vmarea->vm_end;
			zvmarea->next_id = vmarea->vm_next;

			// get file name if any
			if(vmarea->vm_file){
				if((returned_buf_pointer = d_path(&vmarea->vm_file->f_path, file_name_buf, 1024)) > 0){
					strncpy(zvmarea->file_name, returned_buf_pointer, sizeof(zvmarea->file_name));
				}
				zvmarea->file_offset = vmarea->vm_pgoff;
			}

			memcpy(buf+*transfered, zvmarea, zvmarea_size);
			*transfered += zvmarea_size;
		}while(vm_area_id = vmarea->vm_next, vmarea = vmarea->vm_next);

	}

	kfree(file_name_buf);
	kfree(zvmarea);

	// make zeroed space
	fill_with_zeroes(buf+*transfered, zvmarea_size, transfered);

	printk("Total vmarea structures: %u\n", counter);

	return true;
}


static bool dump_page_tables(char *buf, size_t *transfered){
	struct task_struct *task;
	struct vm_area_struct *vmarea;
	struct mm_walk mwalk = {
		pgd_entry: zim32_pgd_callback,
		pud_entry: zim32_pud_callback,
		pmd_entry: zim32_pmd_callback,
		pte_entry: zim32_pte_callback,
		private: 0
	};

	for_each_process(task) {

		if(!task->mm || !task->mm->mmap){
			continue;
		}

		mwalk.mm = task->mm;

		vmarea = task->mm->mmap;

		do{
			if(is_vm_hugetlb_page(vmarea)){
				printk("Huge pages VMA is not supported\n");
				continue;
			}

			mwalk.private = vmarea;

			if(!zim32_walk_page_range(vmarea->vm_start, vmarea->vm_end, &mwalk)){
				return false;
			}

		}while((vmarea = vmarea->vm_next));

	}

	return true;
}


static int dump_all(void *data){
	stop_machine_data_t *args = (stop_machine_data_t *)data;

	if(
			!dump_processes(global_buf, args->total_tranfered) ||
			!dump_mm(global_buf, args->total_tranfered) ||
			!dump_vm_areas(global_buf, args->total_tranfered) ||
			!dump_page_tables(global_buf, args->total_tranfered)
	){
		return -1;
	}

	return 0;
}


static ssize_t read_proc(struct file *filp, char *buf, size_t count, loff_t *offp)
{
	size_t can_not_transfer;
	size_t transfered_succesfully = 0;
	bool end = false;

	printk("Count: %lu\n", count);
	printk("Offset: %lu\n", (unsigned long)*offp);


	if(*offp + count >= total_dumped){
		count = total_dumped - *offp;
		end = true;
	}

	can_not_transfer = copy_to_user(buf, global_buf+*offp, count);
	if(can_not_transfer > 0){
		printk("Can not transfer %zu bytes\n", can_not_transfer);
	}
	transfered_succesfully = count - can_not_transfer;
	*offp += transfered_succesfully;

	if(end){
		return 0;
	}

	printk("Total transfered: %zu\n", transfered_succesfully);

	return transfered_succesfully;
}


static struct proc_dir_entry *proc_entry;
static struct file_operations proc_fops = {
	read: read_proc
};



static int __init hello_init(void)
{
	stop_machine_data_t args;
	args.total_tranfered = &total_dumped;

	global_buf = (char *)vmalloc(ALLOCATED_PAGES_NUM*PAGE_SIZE);
	if(!global_buf){
		printk("Can not allocate memory");
		return -ENOMEM;
	}

	if(stop_machine(dump_all, &args, 0) != 0){
		printk("Error while dumping\n");
		vfree(global_buf);
		return -ENOMEM;
	}

    printk("Dumped %zu bytes\n", total_dumped);
    printk("Mask %lu\n", PTE_PFN_MASK);

    proc_entry = proc_create_data("zim32", 0, NULL, &proc_fops, NULL);

    return 0;    // Non-zero return means that the module couldn't be loaded.
}

static void __exit hello_cleanup(void)
{
    printk(KERN_INFO "Cleaning up module.\n");
    remove_proc_entry("zim32", NULL);
    vfree(global_buf);
}

module_init(hello_init);
module_exit(hello_cleanup);
