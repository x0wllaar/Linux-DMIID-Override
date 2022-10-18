# Linux Kernel DMI Override

A small kernel module (or, rather module template)
that can help you change DMI variables in the kernel
and in sysfs. This can be helpful if your vendor 
forgot to set them, or set them incorrectly.

This is experimental, and uses [kretprobes](https://www.kernel.org/doc/Documentation/kprobes.txt) to hook the [dmi_get_system_info](https://elixir.bootlin.com/linux/v6.0.2/source/drivers/firmware/dmi_scan.c#L928) kernel function. It then replaces the values returned from the function with the internal values of the module.

This is the same approach as this not accepted patch 
https://lore.kernel.org/lkml/20220206214345.31214-1-sp1rit@national.shitposting.agency/T/,
but done with runtime code injection, and on a lower level function.

## Warning

This module uses a debug interface to hook and modify a 
low-level kernel function. This can obviously lead to 
instability and other things. It also hooks a function 
that is used ina lot of internal things in the kernel (mainly 
to apply hardware-specific hacks), and thus can have unintended 
side effects. On the other hand, it can be easily used to alter
the behavior of kernel internals, which can be desirable.

In general, use with care, and understand that this is
experimental.

## How to use

Clone this repo.

Modify the `overrides.h` file so that it contains overrides
for the values you want to change. Their names correspond
to the variable names in DMI, field names in the kernel,
and file names in `/sys/class/dmi/id`. When a value in
`overrides.h` is set to `""`, the module does not change
the real value.

Compile the module with `make`. Make sure that you have
the headers for your kernel installed.

Load the `dmi_override.ko` module with insmod.

Check that you can see the overrides in `/sys/class/dmi/id`

## Todo

- DKMS Support
- Take overrides as arguments