/*
 * Arnewsh 5206 ColdFire system emulation.
 *
 * Copyright (c) 2007 CodeSourcery.
 *
 * This code is licensed under the GPL
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu-common.h"
#include "cpu.h"
#include "hw/hw.h"
#include "hw/m68k/mcf.h"
#include "hw/boards.h"
#include "hw/loader.h"
#include "elf.h"
#include "exec/address-spaces.h"
#include "qemu/error-report.h"
#include "sysemu/qtest.h"

#define KERNEL_LOAD_ADDR 0x10000
#define AN5206_RAM_ADDR 0x10000000
#define AN5206_SRAM_ADDR 0x80000000
#define AN5206_MBAR_ADDR 0xf0000000
#define AN5206_RAMBAR_ADDR 0x40000000

/* Board init.  */

static void an5206_init(MachineState *machine)
{
    ram_addr_t ram_size = machine->ram_size;
    const char *kernel_filename = machine->kernel_filename;
    M68kCPU *cpu;
    CPUM68KState *env;
    int kernel_size;
    uint64_t elf_entry;
    hwaddr entry;
    MemoryRegion *address_space_mem = get_system_memory();
    MemoryRegion *flash = g_new(MemoryRegion, 1);
    MemoryRegion *ram = g_new(MemoryRegion, 1);
    MemoryRegion *rambar = g_new(MemoryRegion, 1);
    MemoryRegion *sram = g_new(MemoryRegion, 1);

    cpu = M68K_CPU(cpu_create(machine->cpu_type));
    env = &cpu->env;

    /* Initialize CPU registers.  */
    env->vbr = 0;
    /* TODO: allow changing MBAR and RAMBAR.  */
    env->mbar = AN5206_MBAR_ADDR | 1;
    env->rambar0 = AN5206_RAMBAR_ADDR | 1;

    /* Flash at address zero */
    memory_region_allocate_system_memory(flash, NULL, "an5206.flash", ram_size);
    memory_region_add_subregion(address_space_mem, 0, flash);

    memory_region_allocate_system_memory(ram, NULL, "an5206.ram", ram_size);
    assert (ram_size > 0x1000000);
    memory_region_add_subregion(address_space_mem, AN5206_RAM_ADDR, ram);

    memory_region_allocate_system_memory(rambar, NULL, "an5206.rambar", ram_size);
    assert (ram_size > 0x1000000);
    memory_region_add_subregion(address_space_mem, AN5206_RAMBAR_ADDR, rambar);

    /* Internal SRAM.  */
    memory_region_init_ram(sram, NULL, "an5206.sram", 512*16, &error_fatal);
    memory_region_add_subregion(address_space_mem, AN5206_SRAM_ADDR, sram);

    mcf5206_init(address_space_mem, AN5206_MBAR_ADDR, cpu);

    char *fn;
    uint8_t *ptr;

    const size_t ROM_SIZE = 0x200000;

    /* Load firmware */
    if (machine->firmware) {
        fn = qemu_find_file(QEMU_FILE_TYPE_BIOS, machine->firmware);
        if (!fn) {
            error_report("Could not find ROM image '%s'", machine->firmware);
            exit(1);
        }
        if (load_image_targphys(fn, 0x0, ROM_SIZE) < 8) {
            error_report("Could not load ROM image '%s'", machine->firmware);
            exit(1);
        }
        g_free(fn);
        /* Initial PC is always at offset 4 in firmware binaries */
        ptr = rom_ptr(0x4, 4);
        assert(ptr != NULL);
        env->pc = ldl_p(ptr);
    }

    /* Load kernel.  */
    if (!kernel_filename) {
        if (qtest_enabled() || machine->firmware) {
            return;
        }
        error_report("Kernel image must be specified");
        exit(1);
    }


    kernel_size = load_elf(kernel_filename, NULL, NULL, &elf_entry,
                           NULL, NULL, 1, EM_68K, 0, 0);
    entry = elf_entry;
    if (kernel_size < 0) {
        kernel_size = load_uimage(kernel_filename, &entry, NULL, NULL,
                                  NULL, NULL);
    }
    if (kernel_size < 0) {
        kernel_size = load_image_targphys(kernel_filename, KERNEL_LOAD_ADDR,
                                          ram_size - KERNEL_LOAD_ADDR);
        entry = KERNEL_LOAD_ADDR;
    }
    if (kernel_size < 0) {
        error_report("Could not load kernel '%s'", kernel_filename);
        exit(1);
    }

    env->pc = entry;
}

static void an5206_machine_init(MachineClass *mc)
{
    mc->desc = "Arnewsh 5206";
    mc->init = an5206_init;
    mc->default_cpu_type = M68K_CPU_TYPE_NAME("m5206");
}

DEFINE_MACHINE("an5206", an5206_machine_init)
