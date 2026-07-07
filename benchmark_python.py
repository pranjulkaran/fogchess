import time
from fog_sdk import FogEngine

BATCHES = 2500  # 10,000 parallel games
STEPS = 1000
TOTAL_GAMES = BATCHES * 4

print(f"[Python Benchmark] Initializing {TOTAL_GAMES} parallel games...")
engine = FogEngine(max_batches=BATCHES)

batch_indices = list(range(BATCHES))
engine.reset_batches(batch_indices)

# Array of 10,000 identical commands (e2-e4) for the test
commands = [5121] * TOTAL_GAMES

print(f"[Python Benchmark] Stepping {STEPS} times...")
start_time = time.perf_counter()

for _ in range(STEPS):
    engine.step_batches(batch_indices, commands)

end_time = time.perf_counter()
elapsed = end_time - start_time
total_steps = TOTAL_GAMES * STEPS
sps = total_steps / elapsed

print("-" * 48)
print(f"Total Environment Steps: {total_steps:,}")
print(f"Time Elapsed:            {elapsed:.3f} seconds")
print(f"Python C-ABI Throughput: {sps:,.0f} SPS (Steps Per Second)")
print("-" * 48)