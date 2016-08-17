#ifndef INCLUDE_TYPES_H_ZIM32
#define INCLUDE_TYPES_H_ZIM32


enum TASK_STATE {
	RUNNING, INTERRUPTIBLE, UNINTERRUPTIBLE
};

enum PAGE_TABLE_ENTRY_TYPE {
	PGD, PUD, PMD, PTE
};


typedef struct stop_machine_data {
	size_t *total_tranfered;
} stop_machine_data_t;


typedef struct zim32_page_table_entry {
	unsigned long data;
	enum PAGE_TABLE_ENTRY_TYPE type;
	void *vm_area_id;
} zim32_page_table_list_entry_t;


typedef struct zim32_vm_area{
	void *id;
	unsigned long vm_start;
	unsigned long vm_end;
	void *next_id;
	void *mm_id;
	char file_name[150];
	unsigned long file_offset;
} zim32_vm_area_t;


typedef struct zim32_mm{
	void *id;
	void *task_id;
	zim32_page_table_list_entry_t pgd;
	unsigned long start_code, end_code, start_data, end_data, start_brk, brk, start_stack;
} zim32_mm_t;

typedef struct zim32_task {
	void *id;
	char cmd[20];
	enum TASK_STATE state;
	void *mm_id;
} zim32_task_t;



#endif /* INCLUDE_TYPES_H_ */
