# STATE.md — Project Memory

## Current Status
- Phase: 0 (Planning complete, Phase 1 not started)
- Milestone: 1

## Repo Baseline (what we found)

### Active Cache Files (what we build on)
- Quartus/rtl/common/ICache2Way.sv    — 2-way set-assoc I-cache, has stub coherence ports
- Quartus/rtl/common/DCache2Way.sv   — 2-way set-assoc D-cache, has LRU + victim writeback
- Quartus/rtl/common/HarvardArbiter3Way.sv — 3-way priority arbiter, what we modify
- Quartus/rtl/common/HarvardCacheSystem.sv — top-level, one cache pair, we create dual-core version

### Reference Only (do not modify)
- Quartus/rtl/common/ICache.sv       — direct-mapped, superseded
- Quartus/rtl/common/DCache.sv       — direct-mapped, superseded
- Quartus/rtl/common/HarvardArbiter.sv — 2-way arbiter, superseded

### Arbiter Priority (HarvardArbiter3Way)
1. Victim writeback (dirty line eviction) — highest
2. D-cache main writes (flush/fill)
3. Round-robin reads (ICache vs DCache)

### Existing Coherence Stubs in ICache2Way
The coh_wr_valid / coh_wr_addr / coh_probe_* ports already exist but are minimally wired.
Phase 2 replaces/extends these with proper MESI snoop ports.

## Key Design Decisions Locked In
- MESI encoding: M=2'b11, E=2'b10, S=2'b01, I=2'b00
- Snoop invalidation: combinational (same-cycle), not registered
- MESI state update: registered (takes effect next posedge)
- Simulator: Icarus Verilog (consistent with existing test suite)
- Testbench style: UVM-lite (no library), pure SystemVerilog

## New Files to Create
- Quartus/rtl/common/SnoopArbiter.sv         (Phase 3)
- Quartus/rtl/common/DualCoreCacheSystem.sv  (Phase 4)
- Quartus/rtl/common/CpuStub.sv              (Phase 4)
- modelsim/mesi_transaction.sv               (Phase 5)
- modelsim/mesi_generator.sv                 (Phase 5)
- modelsim/mesi_monitor.sv                   (Phase 5)
- modelsim/mesi_scoreboard.sv                (Phase 5)
- modelsim/mesi_tb_top.sv                    (Phase 5)
- modelsim/mesi_coverage.sv                  (Phase 6)
- VERIFICATION_REPORT.md                     (Phase 6)

## Bugs / Notes Found During Planning
- HarvardCacheSystem.sv ties inval_valid=1'b0 (invalidation port unused) — Phase 2 fixes this
- DCache2Way has victim writeback port (vwb_*) — SnoopArbiter must keep this working
