# REQUIREMENTS.md — MESI Snooping Cache + UVM-lite DV

## Functional Requirements

### FR-1: MESI State Tracking
- Each cache line in ICache2Way and DCache2Way must carry a 2-bit MESI state register
- Valid states: M=2'b11, E=2'b10, S=2'b01, I=2'b00
- State must initialize to Invalid (I) on reset
- State transitions must follow the MESI invalidation protocol rules

### FR-2: Snoop Input Ports
- ICache2Way and DCache2Way must each accept:
  - snoop_valid   : 1-bit — a snoop transaction is on the bus this cycle
  - snoop_addr    : 19-bit — address being snooped
  - snoop_is_write: 1-bit — 1 = write (invalidating), 0 = read (possibly sharing)
  - snoop_hit_out : 1-bit output — asserted if this cache has the snooped line

### FR-3: Snoop Hit + Invalidation (Single-Cycle)
- When snoop_valid=1 and snoop_is_write=1 and the snooped address hits in a cache:
  - That cache line's MESI state must transition to I (Invalid) within the SAME clock cycle
  - The cache must NOT serve a read on the next rising edge from a line just invalidated
  - This is the core correctness requirement — stale reads are the bug we are preventing

### FR-4: Arbiter Snoop Broadcasting
- HarvardArbiter3Way (or a new SnoopArbiter.sv) must:
  - Detect when a write transaction is granted to either D-cache
  - Broadcast snoop_valid, snoop_addr, snoop_is_write to ALL connected cache pairs
  - Assert broadcast for exactly 1 cycle when the write is committed

### FR-5: Dual-Core Wrapper
- A new DualCoreCacheSystem.sv must instantiate:
  - Two independent cache pairs (ICache0+DCache0, ICache1+DCache1)
  - One shared arbiter with snoop broadcasting
  - Snoop outputs from arbiter connected to both cache pairs' snoop input ports

### FR-6: MESI State Transitions (complete set)
  Read miss, no other copy:       I → E
  Read miss, another has E or M:  I → S (other: E→S or M→S after writeback)
  Read hit:                       state unchanged (M, E, S all stay)
  Write hit in M:                 M → M (already owner)
  Write hit in E:                 E → M (upgrade, no bus transaction needed)
  Write hit in S:                 S → M (broadcast invalidate to others)
  Write miss:                     I → M (fetch + broadcast invalidate)
  Snoop read (this cache has M):  M → S (writeback to memory first)
  Snoop write (this cache has S): S → I
  Snoop write (this cache has E): E → I
  Snoop write (this cache has M): M → I (writeback + invalidate — M wins on bus)

## Verification Requirements

### VR-1: UVM-lite Transaction Generator
- Constrained-random sequence item with fields:
  - rand logic       is_write
  - rand logic [18:0] addr     // 512 KB address space
  - rand logic [15:0] data
  - rand logic        core_sel // 0 = Core 0, 1 = Core 1
- Constraints:
  - Hot-spot constraint: 40% of addresses must target a fixed 8-address "hot zone"
  - Simultaneous write: 10% of back-to-back pairs target the same address from different cores

### VR-2: Bus Monitor
- Passive module that observes the shared bus (snoop bus output from arbiter)
- Captures every transaction: {cycle, core, addr, data, is_write}
- Feeds observed transactions into the scoreboard

### VR-3: Scoreboard (Reference Model)
- Maintains a "golden" associative array: logic [15:0] golden_mem [logic [18:0]]
- On every write: update golden_mem[addr] = data
- On every read response: compare observed data vs golden_mem[addr]
- On mismatch: $error, print full context (addr, expected, got, cycle), $finish

### VR-4: Functional Coverage
- Covergroup cg_mesi_transitions:
  - All 7 MESI state transitions exercised (see FR-6)
- Covergroup cg_collision:
  - Both cores issue write to same address in same cycle
  - Both cores issue conflicting read+write in same cycle
- Covergroup cg_replacement:
  - All cache sets filled (causes LRU eviction of dirty line)
  - M-state line eviction (triggers writeback)

### VR-5: Stress Test
- At least 10,000 random transactions
- Must achieve 100% functional coverage before passing
- Scoreboard must report zero mismatches

## Non-Functional Requirements

### NFR-1: Timing
- Snoop invalidation must be combinational (same-cycle, not registered)
- MESI state update may be registered (takes effect on next posedge)
- Net effect: write is committed on cycle N, snoop invalidates at cycle N posedge → stale read impossible on cycle N+1

### NFR-2: Area
- MESI adds 2 bits per cache line
- DCache2Way: 256 sets × 2 ways × 2 bits = 1024 bits = 128 bytes overhead
- ICache2Way: same — both acceptable for Cyclone V

### NFR-3: Interface Compatibility
- CPU Core.sv must NOT require modification
- Cache-facing interface (c_addr, c_data_in, c_ack, etc.) unchanged
- New ports are arbiter-facing or snoop-bus-facing only
