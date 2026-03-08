# harvard-mesi-dv

A snooping-based **MESI coherence protocol** implemented on top of a Harvard Architecture
cache system, paired with a **UVM-lite SystemVerilog verification suite** designed to prove
correctness under stress.

Built as a portfolio project to demonstrate **Digital Design** and **Design Verification (DV)**
skills, targeting roles on silicon architecture and verification teams.

---

## What This Project Does

### The Hardware (Design Side)
- Dual-core system: each core has private **ICache** and **DCache** (2-way set-associative, 8 KB each)
- **MESI protocol** injected into each cache: every cache line carries a 2-bit state (Modified / Exclusive / Shared / Invalid)
- **SnoopArbiter**: a modified bus arbiter that broadcasts write transactions to all caches so they can invalidate stale lines in the same clock cycle
- Single-cycle invalidation guarantee: a stale read is impossible on the rising edge after a write

### The Verification (DV Side)
- **Constrained-Random Generator**: drives thousands of transactions with hot-spot address constraints
- **Bus Monitor**: passively observes every transaction on the snoop bus
- **Scoreboard**: golden reference model using SystemVerilog associative arrays — catches any stale read instantly
- **Functional Coverage**: covergroups proving every MESI state transition was exercised, including simultaneous same-address writes from both cores

---

## Architecture

```
     Core 0                Core 1
       |                     |
  [ICache0/DCache0]     [ICache1/DCache1]
  (MESI-enabled)        (MESI-enabled)
       |      snoop bus       |
       +----------+----------+
                  |
           [SnoopArbiter]
                  |
             [SDRAM / Memory]
```

---

## MESI State Machine

| State | Meaning |
|-------|---------|
| M (Modified) | Only this cache has the line. Data is dirty (differs from memory). |
| E (Exclusive) | Only this cache has the line. Data is clean. |
| S (Shared) | Multiple caches may have this line. Data is clean. |
| I (Invalid) | This cache does not have this line. |

Key transitions enforced by snooping:
- Another core writes a line you hold in S or E → your line goes to **I**
- Another core reads a line you hold in M → you write back to memory, both go to **S**

---

## Project Phases

| Phase | Deliverable |
|-------|-------------|
| 1 | MESI state registers added to ICache2Way + DCache2Way |
| 2 | Snoop input ports + same-cycle invalidation logic |
| 3 | SnoopArbiter — broadcasts writes on shared bus |
| 4 | DualCoreCacheSystem — two cores wired to shared arbiter |
| 5 | UVM-lite testbench: Generator, Monitor, Scoreboard |
| 6 | Functional coverage + 10,000-transaction stress test + Verification Report |

---

## Baseline

This project extends the **MyPC** 80186 FPGA emulator by
[Waldo Alvarez](https://github.com/waldoalvarez00/MyPC), reusing the Harvard cache
infrastructure (ICache2Way, DCache2Way, HarvardArbiter3Way) as a starting point.
The MESI protocol, dual-core wrapper, and entire verification suite are original work.

Original project licensed under GPL-3.0. This project inherits that license.

---

## Running Tests

```bash
# Existing cache regression tests
cd modelsim/
python3 test_runner.py --category memory

# MESI verification suite (Phase 5+)
cd modelsim/
python3 test_runner.py --test mesi_tb
```

---

## Tech Stack
- **Language:** SystemVerilog
- **Simulator:** Icarus Verilog
- **FPGA Target:** Intel Cyclone V (MiSTer DE10-Nano)
- **Testbench Style:** UVM-lite (no library dependency)
