import sys
import time
import os
from sentence_transformers import SentenceTransformer

# Dynamic Model Selection
MODEL_NAME = sys.argv[1] if len(sys.argv) > 1 else 'sentence-transformers/all-MiniLM-L6-v2'
QUEUE_FILE = "benchmark.queue"

print(f"Loading model: {MODEL_NAME}")

# OPTIMIZATION 2: High-precision timer for load time
t0 = time.perf_counter()
model = SentenceTransformer(MODEL_NAME)
t1 = time.perf_counter()
load_time = (t1 - t0) * 1000.0
print(f"Model loaded in {load_time:.2f} ms\n")

# OPTIMIZATION 3: The Warm-up
print("Warming up model to absorb PyTorch initialization penalty...")
for _ in range(5):
     _ = model.encode("Warmup")
print("Warmup complete. Waiting for C++ to send data...\n")

if os.path.exists(QUEUE_FILE): 
    os.remove(QUEUE_FILE)

while True:
    if not os.path.exists(QUEUE_FILE):
        time.sleep(0.001)
        continue
        
    # Read the queue
    with open(QUEUE_FILE, "r") as f:
        lines = f.readlines()
        
    if not lines:
        time.sleep(0.001)
        continue

    total_time = 0.0
    min_time = float('inf')
    max_time = 0.0
    dimension = 0
    runs = len(lines)

    for i, line in enumerate(lines):
        text = line.strip()
        if not text: continue

        # OPTIMIZATION 4: High-precision timer for the math
        start = time.perf_counter()
        embedding = model.encode(text)
        end = time.perf_counter()

        elapsed = (end - start) * 1000.0
        total_time += elapsed
        min_time = min(min_time, elapsed)
        max_time = max(max_time, elapsed)
        dimension = len(embedding)

        print(f"Run {i + 1:3} : {elapsed:.2f} ms")

    # Clean up queue file
    os.remove(QUEUE_FILE)

    print("\n----------- Summary -----------")
    print(f"Model            : {MODEL_NAME}")
    print(f"Dimension        : {dimension}")
    print(f"Total time       : {total_time:.2f} ms for {runs} items")
    print(f"Average Time     : {total_time / runs:.2f} ms")
    print(f"Minimum Time     : {min_time:.2f} ms")
    print(f"Maximum Time     : {max_time:.2f} ms")
    print(f"Model Load Time  : {load_time:.2f} ms\n")
