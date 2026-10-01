# Wildbits Jr2 DMA Engine Emulation Accuracy Assessment & Improvement Roadmap

This document provides a comprehensive technical assessment of the TinyVicky II Direct Memory Access (DMA) engine emulation in MAME (`wildbits_jr2.cpp`) compared against the FPGA RTL implementation (`fpga-6809-cores-staging`), the Nitrobotics documentation, and `WILDBITS_JR2_ARCHITECTURE.md`. It outlines discrepancies, timing nuances, and modular options to achieve hardware-accurate emulation.

---

## 1. Executive Summary

The MAME `wbjr2` emulator provides a functional, register-level emulation of the TinyVicky II DMA controller ($FEC0–$FED7) that correctly supports:
- 1D linear transfers with non-contiguous 24-bit length registers (`{$FECF, $FECC, $FECD}`).
- 2D rectangular block transfers (width, height, source stride, destination stride).
- 1D linear fills and 2D block fills (including 16-bit alternating byte fills).
- Complete DMA logic operations (ALU ops 0–4: COPY, OR, AND, XOR, MASK, plus NOT inversion).
- Rising-edge transfer triggers on bit 7, interrupt generation on `INT_DMA0`, and even/odd byte-lane masking.

However, several architectural and timing differences exist between the emulator and the FPGA RTL:
1. **Synchronous Execution vs. Asynchronous Hardware Stalls**: MAME executes transfers synchronously in a single C++ call within `dma_w()`, meaning software cannot observe in-flight transfer status via $FEC1 bit 7.
2. **Missing Vertical Blanking (VBLANK) Gating**: Hardware restricts DMA execution strictly to the first ~42 scanlines of each frame (~1.4 ms / ~8% of the 60 Hz frame). Large transfers in hardware pause across multiple frames; MAME completes them instantaneously.
3. **Address Space Routing**: MAME's `dma_read_byte` and `dma_write_byte` check MMU/`FLASHDIS` status and permit routing into Flash and Cartridge. In hardware, DMA connects directly to the 2 MB physical SRAM bus and cannot touch Flash, Cartridge, or internal FPGA Block RAM.
4. **Double-Speed (16-bit) DMA Copies**: MAME treats bit 6 of $FEC0 as 16-bit fill only. In hardware, bit 6 enables double-speed 16-bit transfers for copies as well, advancing addresses by 2 bytes per clock.
5. **Hardware Fill Truncation Limits**: Fills exceeding ~130 KB truncate early in hardware due to address accumulator bounds; MAME permits transfers up to 16 MB.

---

## 2. FPGA Hardware RTL vs. Current Emulator Comparison

### A. Memory Bus Architecture & Physical Addressing
* **Hardware RTL (`TinyVKY_DMA_Controller.v`, `TinyVicky_MemoryManagementBlock.v`)**:
  - The DMA engine connects directly to the external 2 MB SRAM bus via `VGE_VidMem_Data_i` / `VGE_VidMem_Addy_o`.
  - The DMA controller outputs a 24-bit address, but only bits 20:1 connect to the physical 16-bit SRAM chips (two 8-bit chips: even byte on lane 0, odd byte on lane 1).
  - DMA **bypasses the MMU entirely**. It has no connection to the Flash chip select, the expansion/cartridge select, or internal FPGA Block RAMs ($C0–$C6). DMA addresses are purely flat physical SRAM addresses (`phys_addr & 0x1FFFFF`).
* **Current MAME (`wildbits_jr2.cpp`)**:
  - `dma_read_byte` and `dma_write_byte` evaluate the 8 KB block number and `m_mmu_io_ctrl & 0x04` (`FLASHDIS`).
  - When `FLASHDIS` is 0, addresses in $40–$7F access `m_flash` and $80–$9F access `m_cart`.
  - *Discrepancy*: DMA accesses to $40–$9F in hardware always hit the underlying SRAM, never Flash or Cartridge, regardless of `FLASHDIS`.

### B. Timing, Execution Model & Bus Handshake
* **Hardware RTL (`TinyVKY_DMA_Controller.v`)**:
  - The DMA engine operates on the 100 MHz clock domain (`EngineClk100Mhz_i`).
  - **VBLANK Window**: DMA only runs while `VDMA_Transfer_Time_Available_i` and `VDMA_Trf_Time_Before2Late_i` are asserted (scanlines 0 through ~42).
  - **Handshake & Stalls**: When triggered, the controller raises `DMA_Request` (`Bus_RDY_o`), causing the system to assert 6809 HALT. It waits for the CPU acknowledge (`FNX6809_BA_i & FNX6809_BS_i`) before starting.
  - **Frame Yielding**: If a transfer does not finish before scanline 42, the engine enters `WAIT_NEXT_SOF`, drains its pipeline for 8 clocks (`DMA_Drain == 7`), and releases `DMA_Request`. The CPU resumes running during active video display (scanlines 43–524). At the next Start-of-Frame (SOF), DMA re-arbitrates for the bus and resumes.
  - **Status Register ($FEC1)**: Bit 7 (`TRF_IP`) is a hardware level that remains `1` as long as the transfer is in progress (including while parked between VBLANK windows). It self-clears to `0` upon completion (`END_GEN_INT`).
* **Current MAME (`wildbits_jr2.cpp`)**:
  - `dma_execute()` runs synchronously in a C++ `for` loop inside `dma_w()`.
  - It sets `m_dma_status = 0x80`, completes all reads and writes, calls `m_maincpu->eat_cycles(cycles)`, sets `m_dma_status = 0x00`, and asserts the interrupt.
  - *Discrepancy*: Software polling $FEC1 immediately after trigger will never see bit 7 set to `1`. No multi-frame yielding occurs.

### C. 16-Bit Word / Double-Speed DMA Transfers
* **Hardware RTL (`TinyVKY_DMA_Controller.v`, lines 280–328)**:
  - Bit 6 of $FEC0 (`SixteenBit_Enable` / `Double_Speed_DMA`) enables 16-bit word operations for both copy and fill.
  - In double-speed mode:
    - `Read_Data_Address_Pointer` and `Write_Data_Address_Pointer` increment by `+2` each step.
    - Both SRAM byte-select lines (`LSBn` and `MSBn`) are asserted low simultaneously.
    - End-of-transfer comparisons mask bit 0 (`{addr[23:1], 1'b0} == {stop[23:1], 1'b0}`).
* **Current MAME (`wildbits_jr2.cpp`)**:
  - Bit 6 is checked only for 16-bit fill (`is_16bit_fill`).
  - For copies, MAME always transfers 1 byte per loop iteration (`dst + i`, `src + i`).
  - *Discrepancy*: Double-speed block and linear copies are not emulated as 16-bit word operations.

### D. Logic Operations (ALU) & Transfer Rates
* **Hardware RTL (`TinyVKY_DMA_Controller.v`, `TinyVicky_MemoryManagementBlock.v`)**:
  - The DMA engine has distinct clock-cycle requirements per transfer type:
    - **1D Linear Fill**: 1 clock per byte/word.
    - **1D Linear Copy**: 4 clocks per byte (`VRAM_1D_ST00B` + `VRAM_1D_ST01` + `VRAM_1D_ST02` + `VRAM_1D_ST03`).
    - **Logic Operations**: 8 clocks per byte (`OP_RD_S0`, `OP_RD_S0B`, `OP_RD_S1`, `OP_RD_D0`, `OP_RD_D0B`, `OP_RD_D1`, `OP_WR0`, `OP_WR`). Each byte requires reading source, reading destination, ALU settling, and writing back.
* **Current MAME (`wildbits_jr2.cpp`)**:
  - MAME accurately models all logic operations (COPY, OR, AND, XOR, MASK, NOT).
  - It estimates cycle consumption using rough CPU cycle divisions (`count / 16`, `count / 6`, `count / 5`, `count / 2`).

### E. Hardware Counter Boundaries & 130 KB Truncation
* **Hardware RTL & Architecture Notes**:
  - Due to internal 17-bit accumulator/counter limits in the address and stride computation logic, linear fills exceeding ~130 KB stop early while still asserting completion status.
* **Current MAME (`wildbits_jr2.cpp`)**:
  - Unbounded loop iterating up to 16 MB (`count <= 0xFFFFFF`).

---

## 3. Options for Improving DMA Emulation Accuracy

The following options are organized by implementation complexity and architectural impact:

### Option 1: Physical SRAM Bus Isolation (Low Effort, High Architectural Fidelity)
* **Objective**: Remove non-hardware MMU, Flash, and Cartridge paths from the DMA engine.
* **Details**:
  - Simplify `dma_read_byte` and `dma_write_byte` to directly access `m_ram[phys_addr & 0x1FFFFF]`.
  - Block access to internal FPGA dual-port Block RAMs ($C0–$C6), as the hardware DMA controller is an external SRAM-only bus master.
* **Pros**: Minimal lines of code changed, eliminates unphysical Flash/Cartridge DMA accesses, strictly conforms to FPGA pinouts.

### Option 2: Full Double-Speed 16-Bit Word Support (Low Effort, Spec Compliance)
* **Objective**: Fully implement hardware double-speed DMA copies.
* **Details**:
  - In `dma_execute()`, check `Double_Speed_DMA` (`m_dma_reg[0] & 0x40`) for both copy and fill paths.
  - When enabled:
    - Increment addresses by 2 bytes per step.
    - Read and write 16-bit words across both lanes.
    - Align loop bounds checking to word addresses (`addr & ~1`).
* **Pros**: Fulfills the 16-bit DMA specification for high-throughput graphics blits.

### Option 3: Timer/VBLANK-Gated Asynchronous DMA Engine (Medium Effort, High Timing Fidelity)
* **Objective**: Eliminate instantaneous synchronous DMA; emulate VBLANK window gating, multi-frame transfer yielding, and asynchronous status polling.
* **Details**:
  - Introduce an internal DMA state machine in `wildbits_jr2_state`:
    - `m_dma_busy`: Indicates transfer in progress.
    - `m_dma_src_ptr`, `m_dma_dst_ptr`: Active 24-bit transfer addresses.
    - `m_dma_rem_count`: Remaining bytes to transfer.
    - `m_dma_rem_y`: Current line/stride counter for 2D transfers.
  - In `dma_w(0, data)`:
    - When triggered, validate parameters, latch start addresses, and set `m_dma_status = 0x80`. Do **not** execute the transfer loop immediately.
  - In the video engine's scanline callback (`vky_scanline_cb`) or via an `emu_timer`:
    - Only during scanlines 0 through 42 (vertical blanking), process a quota of bytes corresponding to the 100 MHz clock rate.
    - Outside scanlines 0–42, halt byte processing, allowing the 6809 CPU to execute normally during active display lines.
    - When `m_dma_rem_count` reaches 0:
      - Clear `m_dma_status = 0x00`.
      - Assert `set_irq(0, 0x40)` if interrupt enable (`m_dma_reg[0] & 0x08`) was requested.
* **Pros**: Software that polls $FEC1 bit 7 observes genuine in-flight hardware status. Large transfers automatically yield across multiple 60 Hz display frames.

### Option 4: Cycle-Accurate 100 MHz Bus Arbitration & CPU HALT (Medium/High Effort)
* **Objective**: Replicate the exact 6809 bus handshake and 100 MHz clock domain timings.
* **Details**:
  - When DMA triggers, assert `m_maincpu->set_input_line(M6809_HALT_LINE, ASSERT_LINE)`.
  - Wait for the CPU to signal bus grant (`BA=1, BS=1`).
  - Calculate precise cycle durations based on the hardware state machine:
    - Linear fill: 1 engine clock (10 ns) / byte.
    - Linear copy: 4 engine clocks (40 ns) / byte.
    - Logic op transfer: 8 engine clocks (80 ns) / byte.
    - Post-transfer pipeline drain: 8 engine clocks (80 ns).
  - Scale engine clocks to the 14.318 MHz CPU clock domain ($14.318 / 100 \approx 0.1432$ CPU cycles per engine clock).
* **Pros**: Perfect cycle accounting and true CPU bus suspension matching hardware RTL.

### Option 5: Modeling Hardware Address Bounds & Corner-Case Limits (Low Effort)
* **Objective**: Replicate the ~130 KB fill truncation behavior and address wrap.
* **Details**:
  - Mask linear counter increments to 17 bits ($1FFFF), replicating the internal accumulator overflow that occurs on oversized transfers in hardware.
* **Pros**: Catches software bugs relying on unarchitected transfer lengths.

---

## 4. Recommended Implementation Strategy

For a phased enhancement of the MAME `wbjr2` emulator:

1. **Phase 1 (Immediate / Core Architecture)**:
   - Implement **Option 1 (Physical SRAM Isolation)**: Restrict all DMA operations to `m_ram[phys_addr & 0x1FFFFF]`.
   - Implement **Option 2 (Double-Speed 16-Bit DMA)**: Add 16-bit word-stepping to 1D and 2D copies when bit 6 of $FEC0 is active.
2. **Phase 2 (Timing & Asynchrony)**:
   - Implement **Option 3 (VBLANK-Gated Engine)**: Drive DMA bursts during scanlines 0–42 to support true in-flight polling of $FEC1 bit 7 and multi-frame rendering.
3. **Phase 3 (Hardware Quirks & Validation)**:
   - Add **Option 4** and **Option 5** as required if diagnostic test suites (e.g., `dmaxfer` ed. 4) probe exact cycle latencies or oversized fill bounds.
