# ROADMAP.md — MESI Snooping Cache + UVM-lite DV

## Milestone 1: Working MESI Hardware + Verified Correctness

---

### Phase 1: FSM Audit + MESI State Registers
**Goal:** Understand the existing state machines inside ICache2Way and DCache2Way.
Add the 2-bit MESI state array to each cache line. No snoop logic yet — just state storage.

**Why first:** You can't add snooping until you have something to snoop *into*.
Like adding security locks to a house — you need doors first.

**Deliverables:**
- Annotated diagram of existing ICache2Way and DCache2Way FSMs
- MESI state array added to both caches (2 bits × lines × ways)
- State initialized to I on reset
- Local state transitions wired up (read miss → E, write hit → M, etc.)
  but without any cross-cache awareness yet

**Verification gate:** Simulation shows single-cache MESI state transitions are correct
in isolation. A write sets M, a clean fill sets E.

---

### Phase 2: Snoop Port Injection into Caches
**Goal:** Add snoop input ports to ICache2Way and DCache2Way.
When snoop_valid fires with a matching address, invalidate that line THIS cycle.

**Why second:** Before the arbiter can broadcast, the caches need ears to listen with.

**Teaching moment:** We discuss why invalidation must be combinational (not registered)
and how a 1-cycle snoop window prevents stale reads.

**Deliverables:**
- snoop_valid, snoop_addr, snoop_is_write, snoop_hit_out ports added to both caches
- Combinational invalidation logic: if snoop fires on a resident line → state → I
- snoop_hit_out wired to indicate cache is responding to snoop

**Verification gate:** Directed test: write to addr X from "snoop bus", confirm cache X line
transitions I within same cycle.

---

### Phase 3: Arbiter Snoop Broadcasting
**Goal:** Modify HarvardArbiter3Way to become a SnoopArbiter. When it commits a write
from either D-cache, broadcast snoop_valid + snoop_addr + snoop_is_write to all caches.

**Why third:** The arbiter sees every transaction — it's the natural place to "shout"
on the bus. Like the grocery delivery truck announcing "I just updated the milk supply."

**Teaching moment:** We cover bus-based snooping vs. directory-based snooping.
What makes bus snooping simple but not scalable to 100+ cores.

**Deliverables:**
- New SnoopArbiter.sv (extended HarvardArbiter3Way)
- Outputs: snoop_valid, snoop_addr, snoop_is_write (broadcast to all caches)
- Broadcasts on the exact cycle the write is committed (mem_m_ack + mem_m_wr_en)
- snoop_hit_in input from each cache (arbiter uses this to delay if needed — M writeback case)

**Verification gate:** Directed test: Core 0 writes addr X → confirm snoop_valid fires
on arbiter output that same cycle.

---

### Phase 4: Dual-Core Wrapper (DualCoreCacheSystem.sv)
**Goal:** Instantiate two full cache pairs (Core 0 and Core 1), wire them both to the
SnoopArbiter. Snoop outputs from arbiter go to BOTH cache pairs.

**Why fourth:** This is the moment the "private island" metaphor becomes real hardware.
Two refrigerators, one delivery truck.

**Teaching moment:** We discuss why you can't just share one cache between two cores
(structural hazards, simultaneous access, write conflicts).

**Deliverables:**
- DualCoreCacheSystem.sv instantiating:
  - ICache0 + DCache0 (Core 0's private caches)
  - ICache1 + DCache1 (Core 1's private caches)
  - SnoopArbiter (shared bus)
  - Snoop bus wired to both cache pairs
- Simple CPU stub modules (CpuStub.sv) to drive the cache interfaces for testing

**Verification gate:** Both caches connected and compile cleanly. System-level reset test passes.

---

### Phase 5: UVM-lite Testbench Foundation
**Goal:** Build the three core verification components: Transaction Generator (stimulus),
Bus Monitor (observation), and Scoreboard (judgment).

**Why fifth:** We can't "stress test" until we have something to stress test *with*.
The testbench is the judge — design first, then start judging.

**Teaching moment:** What UVM actually is, why we're doing "lite" (no library overhead),
and how a Scoreboard is the hardware equivalent of a software unit test assertion.

**Deliverables:**
- mesi_transaction.sv — constrained-random transaction class with hot-spot constraints
- mesi_generator.sv — drives randomized traffic to both cores
- mesi_monitor.sv — passive observer on the snoop bus
- mesi_scoreboard.sv — golden model with associative array, mismatch detection
- mesi_tb_top.sv — top-level testbench wiring everything together

**Verification gate:** Testbench compiles and runs 100 directed transactions with zero
scoreboard errors.

---

### Phase 6: Functional Coverage + Stress Testing
**Goal:** Write covergroups, run 10,000+ random transactions, achieve 100% coverage.
Generate the Verification Report.

**Why last:** Coverage proves completeness. You don't know what you haven't tested
until you measure it. This is the final "proof of correctness."

**Teaching moment:** The difference between Code Coverage (did I touch every line?)
and Functional Coverage (did I test every scenario that matters?). Apple cares about
functional coverage.

**Deliverables:**
- mesi_coverage.sv — covergroups for all 7 MESI transitions, collision scenarios, evictions
- Stress test: 10,000+ random transactions with hot-spot constraint active
- VERIFICATION_REPORT.md documenting:
  - Pass/fail summary
  - Coverage percentages per covergroup
  - Any bugs found and fixed during verification
  - Notable race conditions caught by scoreboard

**Verification gate:** 100% functional coverage, 0 scoreboard errors, report written.

---

## Summary Table

| Phase | What We Build | Key Concept Taught |
|-------|--------------|-------------------|
| 1 | MESI state registers in caches | Cache FSMs, state encoding |
| 2 | Snoop ports + invalidation logic | Same-cycle timing, combinational vs registered |
| 3 | SnoopArbiter broadcasting | Bus-based snooping, write commit point |
| 4 | Dual-core wrapper | Multi-core cache topology |
| 5 | UVM-lite testbench components | Scoreboards, monitors, constrained random |
| 6 | Coverage + stress testing | Functional coverage, verification closure |
