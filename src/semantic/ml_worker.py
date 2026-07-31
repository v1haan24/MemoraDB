import time
import os
from sentence_transformers import SentenceTransformer

print("========================================")
print("  MemoraDB Python ML Worker Initializing ")
print("========================================")

start_load = time.time()
model = SentenceTransformer('all-MiniLM-L6-v2')
load_time = (time.time() - start_load) * 1000
print(f"✅ Model loaded in {load_time:.2f} ms")

# Warmup to absorb PyTorch cold-start latency
print("🔥 Warming up model (5 iterations)...")
for _ in range(5):
    _ = model.encode("natural language processing")
print("✅ Warmup complete. Worker active and listening...\n")

queue_file = "tasks.queue"
vec_file = "embeddings.vec"

while True:
    if os.path.exists(queue_file):
        # 1. Read queue
        try:
            with open(queue_file, "r") as f:
                lines = [line.strip() for line in f.readlines() if line.strip()]
            
            # Immediately clear queue file
            os.remove(queue_file)
        except Exception:
            continue
        
        if not lines:
            continue
            
        runs = len(lines)
        print(f"📥 Received {runs} queries from C++ Engine...")
        
        start_ml = time.time()
        
        # 2. Generate 384-dim float32 embeddings
        embeddings = model.encode(lines)
        
        # 3. Write raw binary bytes DIRECTLY to embeddings.vec (No atomic swap)
        with open(vec_file, "wb") as f:
            f.write(embeddings.tobytes())
            
        total_ml_time = (time.time() - start_ml) * 1000
        avg_time = total_ml_time / runs
        
        print("-------------- Python Benchmark Summary --------------")
        print(f"  Batch Size                : {runs} items")
        print(f"  ML Encoding + Binary Write: {total_ml_time:.2f} ms")
        print(f"  Avg Latency Per Vector   : {avg_time:.2f} ms")
        print("------------------------------------------------------\n")
    
    time.sleep(0.005) # 5ms polling loop
