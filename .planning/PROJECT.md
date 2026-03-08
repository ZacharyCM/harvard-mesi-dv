# PROJECT.md — MESI Snooping Cache + UVM-lite DV

## What We're Building

A dual-core hardware system with a snooping-based MESI coherence protocol on top of the existing
Harvard Architecture cache system (ICache2Way + DCache2Way + HarvardArbiter3Way). Paired with a
UVM-lite SystemVerilog verification environment to prove the protocol is correct under stress.

Target: Demonstrate Design Verification (DV) and Digital Design skills for an Apple interview.

## Why This Project Matters for Apple

Apple Silicon (M-series chips) relies heavily on multi-core coherence protocols. Being able to
(a) design a snooping MESI protocol and (b) verify it with constrained-random tests, scoreboards,
and functional coverage is exactly what Apple DV engineers do day-to-day.

## Architecture Overview

```
     Core 0                Core 1
       |                     |
  [ICache0/DCache0]     [ICache1/DCache1]
  (MESI-enabled)        (MESI-enabled)
       |                     |
       +--------+------------+
                |
        [SnoopBus Arbiter]   <-- broadcasts write transactions to all caches
                |
           [SDRAM / Memory]
```

### The MESI States (per cache line)
- M (Modified):  Only this cache has the line. Data is DIRTY (differs from memory).
- E (Exclusive): Only this cache has the line. Data is CLEAN (matches memory).
- S (Shared):    Multiple caches may have this line. Data is CLEAN.
- I (Invalid):   This cache does not have this line (or it was invalidated).

### Key Snooping Behavior
- Core 0 writes a line  → broadcast on snoop bus → Core 1 must invalidate (S/E → I)
- Core 0 reads a line Core 1 has in M → Core 1 writes back to memory first, both go to S
- Core 0 reads a line nobody has → goes to E (Exclusive, only copy)
- Core 0 reads a line Core 1 has in S → both stay/go to S

## Existing Codebase Baseline

The repo already has:
- ICache2Way.sv — 2-way set-assoc I-cache with stub coherence ports (coh_wr_valid, coh_probe_*)
- DCache2Way.sv — 2-way set-assoc D-cache with LRU, victim writeback port
- HarvardArbiter3Way.sv — 3-way priority arbiter (VWB > D-write > round-robin reads)
- HarvardCacheSystem.sv — top-level integration of one I+D cache pair + arbiter

What we are NOT reusing directly (keeping as reference only):
- ICache.sv, DCache.sv — direct-mapped variants, superseded by 2-way versions
- HarvardArbiter.sv — 2-way arbiter, superseded by 3Way

## Deliverable

A Verification Report proving the MESI protocol is unbreakable:

A. Stress Test ("The Hammer")
   - Thousands of constrained-random transactions
   - Address hot-spotting: both cores hammering the same cache line simultaneously
   - Goal: trigger collision/race conditions at the arbiter

B. Scoreboard ("The Judge")
   - Golden reference model using SystemVerilog associative arrays
   - Tracks every write; catches stale reads instantly
   - Simulation stops + error flag on first mismatch

C. Functional Coverage ("The Map")
   - Covergroups proving every MESI transition was exercised:
     M→I, E→S, S→I, I→E, I→S, E→M, S→M
   - Both cores writing same line on same clock edge (simultaneous write)
   - Cache-full scenario forcing LRU replacement

## Tech Stack
- Language: SystemVerilog (design) + SystemVerilog (testbench)
- Simulator: Icarus Verilog (free, already used in repo) or Verilator
- No UVM library dependency — "UVM-lite" means we implement the patterns ourselves

## Key Constraints
- Single shared memory bus (not a ring or mesh) — simplifies snoop broadcast
- Single-cycle invalidation required — stale read must be impossible on next rising edge
- Must work with existing CPU Core.sv interface (transparent replacement)
- FPGA target (MiSTer DE10-Nano) — keep area efficient, no exotic constructs
