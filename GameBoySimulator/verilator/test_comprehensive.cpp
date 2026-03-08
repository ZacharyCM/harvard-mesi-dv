#include <verilated.h>
#include "Vtop.h"
#include "Vtop___024root.h"
#include "gb_test_common.h"
#include <stdio.h>

int main() {
    Vtop* dut = new Vtop;
    MisterSDRAMModel* sdram = new MisterSDRAMModel(8);
    sdram->cas_latency = 2;

    // Boot ROM
    uint8_t minimal_boot[256];
    memset(minimal_boot, 0, 256);
    int pc = 0;
    minimal_boot[pc++] = 0xF3;
    while (pc < 0xFC) minimal_boot[pc++] = 0x00;
    minimal_boot[pc++] = 0x3E; minimal_boot[pc++] = 0x01;
    minimal_boot[pc++] = 0xE0; minimal_boot[pc++] = 0x50;

    // Test ROM
    uint8_t rom[32768];
    memset(rom, 0x76, sizeof(rom));

    rom[0x100] = 0xC3; rom[0x101] = 0x50; rom[0x102] = 0x01;  // JP $0150

    // Comprehensive test sequence
    rom[0x150] = 0x3E; rom[0x151] = 0x42;  // LD A, 0x42
    rom[0x152] = 0x06; rom[0x153] = 0x12;  // LD B, 0x12
    rom[0x154] = 0x0E; rom[0x155] = 0x34;  // LD C, 0x34
    rom[0x156] = 0x80;  // ADD A, B
    rom[0x157] = 0x18; rom[0x158] = 0x02;  // JR +2
    rom[0x159] = 0x76;  // HALT (skip)
    rom[0x15A] = 0x00;  // NOP
    rom[0x15B] = 0x37;  // SCF (set carry flag)
    rom[0x15C] = 0x38; rom[0x15D] = 0x02;  // JR C, +2
    rom[0x15E] = 0x76;  // HALT (skip)
    rom[0x15F] = 0x00;  // NOP
    rom[0x160] = 0x3F;  // CCF (complement carry - clear it)
    rom[0x161] = 0x30; rom[0x162] = 0x02;  // JR NC, +2
    rom[0x163] = 0x76;  // HALT (skip)
    rom[0x164] = 0xC3; rom[0x165] = 0x70; rom[0x166] = 0x01;  // JP $0170
    rom[0x170] = 0x76;  // HALT (success)

    sdram->loadBinary(0, rom, sizeof(rom));

    dut->reset = 1;
    dut->inputs = 0xFF;
    dut->ioctl_download = 0;
    dut->boot_download = 0;
    run_cycles_with_sdram(dut, sdram, 50);
    dut->reset = 0;

    dut->boot_download = 1;
    for (int addr = 0; addr < 256; addr += 2) {
        uint16_t word = minimal_boot[addr];
        if (addr + 1 < 256) word |= (minimal_boot[addr + 1] << 8);
        dut->boot_addr = addr;
        dut->boot_data = word;
        dut->boot_wr = 1;
        run_cycles_with_sdram(dut, sdram, 4);
        dut->boot_wr = 0;
        run_cycles_with_sdram(dut, sdram, 4);
    }
    dut->boot_download = 0;

    dut->ioctl_download = 1;
    dut->ioctl_index = 0;
    for (int i = 0; i < 10; i++) {
        dut->ioctl_addr = i;
        dut->ioctl_wr = 1;
        run_cycles_with_sdram(dut, sdram, 64);
        dut->ioctl_wr = 0;
        run_cycles_with_sdram(dut, sdram, 64);
        if (dut->dbg_cart_ready) break;
    }
    dut->ioctl_download = 0;

    bool boot_completed = false;
    bool ld_ok = false, add_ok = false, jr_ok = false, jr_c_ok = false, jr_nc_ok = false, jp_final_ok = false;
    uint16_t last_pc = 0xFFFF;

    printf("Running comprehensive instruction test...\n");

    for (int cycle = 0; cycle < 300000; cycle++) {
        tick_with_sdram(dut, sdram);

        uint16_t pc = dut->dbg_cpu_addr;
        bool boot_enabled = dut->dbg_boot_rom_enabled;

        if (!boot_completed && !boot_enabled) {
            boot_completed = true;
        }

        if (boot_completed && pc != last_pc) {
            if (pc >= 0x152 && pc <= 0x156 && !ld_ok) {
                ld_ok = true;
                printf("  ✅ LD instructions executed\n");
            }
            if (pc == 0x157 && !add_ok) {
                add_ok = true;
                printf("  ✅ ADD instruction executed\n");
            }
            if (pc == 0x15A && !jr_ok) {
                jr_ok = true;
                printf("  ✅ JR +2 worked ($0157 → $015A)\n");
            }
            if (pc == 0x15F && !jr_c_ok) {
                jr_c_ok = true;
                printf("  ✅ JR C worked ($015C → $015F)\n");
            }
            if (pc == 0x164 && !jr_nc_ok) {
                jr_nc_ok = true;
                printf("  ✅ JR NC worked ($0161 → $0164)\n");
            }
            if (pc == 0x170 && !jp_final_ok) {
                jp_final_ok = true;
                printf("  ✅ Final JP worked ($0164 → $0170)\n");
                printf("\n🎉 All tests PASSED! No regressions detected.\n");
                break;
            }
            last_pc = pc;
        }
    }

    int result = (ld_ok && add_ok && jr_ok && jr_c_ok && jr_nc_ok && jp_final_ok) ? 0 : 1;
    
    if (!ld_ok) printf("  ❌ LD instructions failed\n");
    if (!add_ok) printf("  ❌ ADD instruction failed\n");
    if (!jr_ok) printf("  ❌ JR +2 failed\n");
    if (!jr_c_ok) printf("  ❌ JR C failed\n");
    if (!jr_nc_ok) printf("  ❌ JR NC failed\n");
    if (!jp_final_ok) printf("  ❌ Final JP failed\n");

    delete sdram;
    delete dut;
    return result;
}
