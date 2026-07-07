from fog_sdk import FogEngine

print("Booting C++ Engine via Python ctypes...")
engine = FogEngine(max_batches=10)

print("Resetting Batch 0 (4 parallel games)...")
engine.reset_batches([0])

print("Requesting legal moves for Batch 0, Lane 0...")
moves = engine.get_legal_moves(batch_index=0, lane=0)

print(f"Success! Python received {len(moves)} opening moves directly from the C++ Kernel.")
print(f"Raw packed 32-bit commands: {moves[:5]}...")