#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ktime.h>
#include <linux/limits.h>
#include <linux/sched.h>
#include <linux/dmi.h>
#include <linux/device.h>
#include <linux/slab.h>

#include "overrides.h"

#define OVERRIDE_FUNC_NAME "dmi_get_system_info"

//See https://lore.kernel.org/all/20220206214345.31214-1-sp1rit@national.shitposting.agency/
//An array containing overrides for all DMI IDs
static const char* dmi_overrides[DMI_STRING_MAX + 2] = {
    OV_DMI_NONE,
    OV_DMI_BIOS_VENDOR,
    OV_DMI_BIOS_VERSION,
    OV_DMI_BIOS_DATE,
    OV_DMI_BIOS_RELEASE,
    OV_DMI_EC_FIRMWARE_RELEASE,
    OV_DMI_SYS_VENDOR,
    OV_DMI_PRODUCT_NAME,
    OV_DMI_PRODUCT_VERSION,
    OV_DMI_PRODUCT_SERIAL,
    OV_DMI_PRODUCT_UUID,
    OV_DMI_PRODUCT_SKU,
    OV_DMI_PRODUCT_FAMILY,
    OV_DMI_BOARD_VENDOR,
    OV_DMI_BOARD_NAME,
    OV_DMI_BOARD_VERSION,
    OV_DMI_BOARD_SERIAL,
    OV_DMI_BOARD_ASSET_TAG,
    OV_DMI_CHASSIS_VENDOR,
    OV_DMI_CHASSIS_TYPE,
    OV_DMI_CHASSIS_VERSION,
    OV_DMI_CHASSIS_SERIAL,
    OV_DMI_CHASSIS_ASSET_TAG,
    OV_DMI_STRING_MAX,
    OV_DMI_OEM_STRING,
};


struct func_data {
    int field;
};


static int entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    //Skip kernel threads
    if (!current->mm)
        return 1;

    struct func_data *data = (struct func_data *)ri->data;

    //Save function parameters, take them directly from registers
    //See x86-64 calling convention for kernel
    //%rdi  %rsi  %rdx || %r10 %r8 %r9
    data->field = (int)(regs->di);
    pr_info("accesing DMI field %d start", data->field);

    //Give control back to the function
    return 0;
}

static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    //The function is done here, load the data we saved at the beginning
    struct func_data *data = (struct func_data *)ri->data;
    pr_info("accesing DMI field %d end", data->field);
    //See https://elixir.bootlin.com/linux/v6.0.2/source/drivers/firmware/dmi_scan.c#L928
    //Get field number from the saved function arguments
    int field = data->field;
    //If we defined an override in the header file, the length of the string here
    //Will be more than 0
    //If we didn't, it's 0 and we do nothing to the return value
    if (strlen(dmi_overrides[field]) > 0){
        //An override of a field was defined
        pr_info("field access intercepted for DMI field %d", field);
        //Replace the returned char pointer with pointer
        //to our overrides array
        regs->ax = (unsigned long)(dmi_overrides[field]);
    }
    return 0;
}

static struct kretprobe dmiov_kretprobe = {
    .handler          = ret_handler,
    .entry_handler    = entry_handler,
    .data_size        = sizeof(struct func_data),
    //set maxactive to 0, so we try and hook all calls
    //in not RT kernels
    .maxactive        = 0,
};


int init_module(void) {
    //hook function that accesses DMI fields
    dmiov_kretprobe.kp.symbol_name = OVERRIDE_FUNC_NAME;
    int ret = register_kretprobe(&dmiov_kretprobe);
    if (ret < 0) {
        pr_info("register_kretprobe for %s failed, returned %d\n", dmiov_kretprobe.kp.symbol_name, ret);
        return -1;
    }
    pr_info("registered kretprobe at %s: %p\n", dmiov_kretprobe.kp.symbol_name, dmiov_kretprobe.kp.addr);
    return 0;
}

void cleanup_module(void) {
    //unhook
    unregister_kretprobe(&dmiov_kretprobe);
    pr_info("kretprobe at %p unregistered\n", dmiov_kretprobe.kp.addr);
    if (dmiov_kretprobe.nmissed > 0){
        pr_warn("missed hijacking %d instances of %s, THIS SHOULD NEVER HAPPEN\n",
            dmiov_kretprobe.nmissed, dmiov_kretprobe.kp.symbol_name);
    }
}

MODULE_LICENSE("GPL");
