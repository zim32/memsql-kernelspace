#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x634251a8, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x63dba7b, __VMLINUX_SYMBOL_STR(remove_proc_entry) },
	{ 0xc3889f8b, __VMLINUX_SYMBOL_STR(proc_create_data) },
	{ 0x999e8297, __VMLINUX_SYMBOL_STR(vfree) },
	{ 0xb4759adc, __VMLINUX_SYMBOL_STR(stop_machine) },
	{ 0xd6ee688f, __VMLINUX_SYMBOL_STR(vmalloc) },
	{ 0xdb7305a1, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x9166fada, __VMLINUX_SYMBOL_STR(strncpy) },
	{ 0xcacf9c39, __VMLINUX_SYMBOL_STR(d_path) },
	{ 0x90b0859f, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x6711f38d, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0xe914e41e, __VMLINUX_SYMBOL_STR(strcpy) },
	{ 0x295cef8c, __VMLINUX_SYMBOL_STR(pv_mmu_ops) },
	{ 0x15aa96c9, __VMLINUX_SYMBOL_STR(init_task) },
	{ 0x4f8b5ddb, __VMLINUX_SYMBOL_STR(_copy_to_user) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "2573D3B84152E72CF6BF5BD");
