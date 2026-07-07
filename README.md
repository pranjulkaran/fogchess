FogEngine
FogEngine is a high-performance, deterministic simulation runtime designed for turn-based strategy games.

From Prototype to Engine
FogEngine is the direct architectural evolution of the POMDP-64 engine. While the original POMDP-64 project provided the conceptual foundation for bitboard-based chess simulation and RL integration, FogEngine takes those concepts and rebuilds them into a production-grade, data-oriented runtime.

Key advancements over the original POMDP-64 architecture:

SIMD Batching: Moving from single-game processing to a 4-lane (and extensible) batched SoA (Structure of Arrays) architecture for massive parallel throughput.

Deterministic Kernel: Complete isolation of the simulation kernel, ensuring bit-perfect reproducibility across all hardware.

Zero-Allocation Memory: Moving from heap-managed objects to pre-allocated Arena/Pool memory for maximum cache efficiency.

SDK-Driven Design: Decoupling the simulation from the Python/RL layer through a stable, language-agnostic C-ABI.