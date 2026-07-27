import time
import os
import struct
from sentence_transformers import SentenceTransformer

QUEUE_FILE = "outbox.queue"
VEC_FILE = "embeddings.vec"
TARGET_INSERTS = 100 

open(QUEUE_FILE, 'a').close()

print("[Python Worker] Booting up ML Engine...")

# 1. Benchmark Model Load Time
start_load = time.time()
model = SentenceTransformer('all-MiniLM-L6-v2')
end_load = time.time()
model_load_time_ms = (end_load - start_load) * 1000

print(f"[Python Worker] Model loaded. Listening to {QUEUE_FILE}...")

processed = 0
times = []

# Open the .vec file in binary append mode
with open(VEC_FILE, "ab") as vec_out:
    with open(QUEUE_FILE, "r") as f:
        while processed < TARGET_INSERTS:
            line = f.readline()
            
            if not line:
                time.sleep(0.001) 
                continue
                
            # Parse the queue line (Format: PK_ID|Text)
            parts = line.strip().split("|", 1)
            if len(parts) != 2:
                continue
                
            pk, text = parts[0], parts[1]
                
            # Benchmark Individual Encoding Time
            start_encode = time.time()
            vector = model.encode(text) 
            end_encode = time.time()
            
            # Save to embeddings.vec (Write PK length, PK, then float array)
            pk_bytes = pk.encode('utf-8')
            vec_out.write(struct.pack('Q', len(pk_bytes))) # 8-byte length
            vec_out.write(pk_bytes)
            vec_out.write(vector.tobytes()) # 384 floats * 4 bytes = 1536 bytes
            
            times.append((end_encode - start_encode) * 1000)
            processed += 1

# Calculate Stats
total_time_ms = sum(times)
avg_time_ms = total_time_ms / len(times)
min_time_ms = min(times)
max_time_ms = max(times)

# Print 
print("\n" + "-" * 12 + " Summary " + "-" * 12)
print(f"Model           : sentence-transformers/all-MiniLM-L6-v2")
print(f"Dimension       : 384")
print(f"Total time      : {total_time_ms:.2f} ms for {processed} items")
print(f"Average Time    : {avg_time_ms:.2f} ms")
print(f"Minimum Time    : {min_time_ms:.2f} ms")
print(f"Maximum Time    : {max_time_ms:.2f} ms")
print(f"Model Load Time : {model_load_time_ms:.2f} ms")
print("-" * 33)

# Cleanup queue file after processing
os.remove(QUEUE_FILE)