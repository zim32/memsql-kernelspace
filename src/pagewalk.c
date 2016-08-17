#include <linux/mm.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include "../include/types.h"
#include <asm/pgtable.h>

static char *global_buf;
static size_t total_dumped;

static int zim32_pgd_callback(pgd_t *pgd, unsigned long addr, unsigned long next, struct mm_walk *walk){
	zim32_page_table_list_entry_t zpte_entry;
	size_t zpte_size = sizeof(zim32_page_table_list_entry_t);
	pgd_t pgdval = *pgd;

	printk("PGD entry callback\n");

	memset(&zpte_entry, 0, zpte_size);

	zpte_entry.type = PGD;
	zpte_entry.data = pgd_val(pgdval);
	zpte_entry.vm_area_id = walk->private;

	if(total_dumped + zpte_size > GLOBAL_BUFFER_SIZE){
		printk("Buffer overflow");
		return -1;
	}
	memcpy(global_buf+total_dumped, &zpte_entry, zpte_size);
	total_dumped += zpte_size;
	return 0;
}

static int zim32_pud_callback(pud_t *pud, unsigned long addr, unsigned long next, struct mm_walk *walk){
	zim32_page_table_list_entry_t zpte_entry;
	size_t zpte_size = sizeof(zim32_page_table_list_entry_t);
	pud_t pudval = *pud;

	printk("PUD entry callback\n");

	memset(&zpte_entry, 0, zpte_size);

	zpte_entry.type = PUD;
	zpte_entry.data = pud_val(pudval);
	zpte_entry.vm_area_id = walk->private;

	if(total_dumped + zpte_size > GLOBAL_BUFFER_SIZE){
		printk("Buffer overflow");
		return -1;
	}
	memcpy(global_buf+total_dumped, &zpte_entry, zpte_size);
	total_dumped += zpte_size;
	return 0;
}

static int zim32_pmd_callback(pmd_t *pmd, unsigned long addr, unsigned long next, struct mm_walk *walk){
	zim32_page_table_list_entry_t zpte_entry;
	size_t zpte_size = sizeof(zim32_page_table_list_entry_t);
	pmd_t pmdval = *pmd;

	printk("PMD entry callback\n");

	memset(&zpte_entry, 0, zpte_size);

	zpte_entry.type = PMD;
	zpte_entry.data = pmd_val(pmdval);
	zpte_entry.vm_area_id = walk->private;

	if(total_dumped + zpte_size > GLOBAL_BUFFER_SIZE){
		printk("Buffer overflow");
		return -1;
	}
	memcpy(global_buf+total_dumped, &zpte_entry, zpte_size);
	total_dumped += zpte_size;
	return 0;
}

static int zim32_pte_callback(pte_t *pte, unsigned long addr, unsigned long next, struct mm_walk *walk){
	zim32_page_table_list_entry_t zpte_entry;
	size_t zpte_size = sizeof(zim32_page_table_list_entry_t);
	pte_t pteval = *pte;

	printk("PTE entry callback\n");
	if(pte_present(pteval)){
		if(pte_huge(pteval)){
			printk("Huge PTE is not supported\n");
		}else{
			printk("PTE data: %lu\n", pte_val(pteval));
			printk("PFN number: %lu\n", pte_pfn(pteval));
			printk("Physical address: %lu\n", (pte_pfn(pteval) << PAGE_SHIFT));
		}
	}else{
		printk("Page is not present\n");
	}

	memset(&zpte_entry, 0, zpte_size);

	zpte_entry.type = PTE;
	zpte_entry.data = pte_val(pteval);
	zpte_entry.vm_area_id = walk->private;

	if(total_dumped + zpte_size > GLOBAL_BUFFER_SIZE){
		printk("Buffer overflow");
		return -1;
	}
	memcpy(global_buf+total_dumped, &zpte_entry, zpte_size);
	total_dumped += zpte_size;
	return 0;
}



static bool zim32_walk_pte_range(pmd_t *pmd, unsigned long start_address, unsigned long end_address, struct mm_walk *mwalk){
	 pte_t *pte;
	 unsigned long address = start_address;

	 pte = pte_offset_map(pmd, address);

	 for(;;){
		 address += PAGE_SIZE;
		 if(address >= end_address){
			 break;
		 }
		 if(!pte_none(*pte)){
			 if(mwalk->pte_entry){
				 if(mwalk->pte_entry(pte, address, address + PAGE_SIZE, mwalk) != 0){
					 return false;
				 }
			 }
		 }
		 pte++;
	 }

	 pte_unmap(pte);

	 return true;
}



static bool zim32_walk_pmd_range(pud_t *pud, unsigned long start_address, unsigned long end_address, struct mm_walk *mwalk){
	 pmd_t *pmd;
	 unsigned long next;
	 unsigned long address = start_address;

	 pmd = pmd_offset(pud, address);

	 do{
		 next = pmd_addr_end(address, end_address);

		 if(!pmd_none(*pmd) && !pmd_bad(*pmd)){
			 if(mwalk->pmd_entry){
				 if(mwalk->pmd_entry(pmd, address, next, mwalk) != 0){
					 return false;
				 }
			 }
			 if(!zim32_walk_pte_range(pmd, address, next, mwalk)){
				 return false;
			 }
		 }

	 }while(pmd++, address = next, address != end_address);

	 return true;
}


static bool zim32_walk_pud_range(pgd_t *pgd, unsigned long start_address, unsigned long end_address, struct mm_walk *mwalk){
	 pud_t *pud;
	 unsigned long next;
	 unsigned long address = start_address;

	 pud = pud_offset(pgd, address);

	 do{
		 next = pud_addr_end(address, end_address);

		 if(!pud_none(*pud) && !pud_bad(*pud)){
			 if(mwalk->pud_entry){
				 if(mwalk->pud_entry(pud, address, next, mwalk) != 0){
					 return false;
				 }
			 }
			 if(!zim32_walk_pmd_range(pud, address, next, mwalk)){
				 return false;
			 }
		 }

	 }while(pud++, address = next, address != end_address);

	 return true;
}


static bool zim32_walk_page_range(unsigned long start_address, unsigned long end_address, struct mm_walk *mwalk){
	unsigned long next;
	unsigned long address = start_address;
	pgd_t *pgd;

	pgd = pgd_offset(mwalk->mm, address);

	do{
		next = pgd_addr_end(address, end_address);

		if(!pgd_none(*pgd) && !pgd_bad(*pgd)){
			if(mwalk->pgd_entry){
				if(mwalk->pgd_entry(pgd, address, next, mwalk) != 0){
					return false;
				}
			}
			if(!zim32_walk_pud_range(pgd, address, next, mwalk)){
				return false;
			}
		}else{
			pgd++;
			continue;
		}
	}while(
			address = next,
			address < end_address
	);

	return true;
}
