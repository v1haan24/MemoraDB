import sys
import time
import os
from sentence_transformers import SentenceTransformer

# Configuration
if len(sys.argv) < 2:
    print("Usage: python ml_worker.py <table_name>")
    sys.exit(1)
 
TABLE_NAME = sys.argv[1]

QUEUE_DIR = os.path.join("data", TABLE_NAME, "queue")
os.makedirs(QUEUE_DIR, exist_ok=True)

QUEUE_FILE = os.path.join(QUEUE_DIR, "tasks.queue")
TEMP_VEC_FILE = os.path.join(QUEUE_DIR, "tempEmbed.vec")
VEC_FILE = os.path.join(QUEUE_DIR, "embeddings.vec")
DONE_FILE = os.path.join(QUEUE_DIR, "done.signal")

POLL_INTERVAL = 0.001   # 1 ms

# Load model
print("========================================")
print(f"  MemoraDB Python ML Worker Initializing ({TABLE_NAME})")
print("========================================")

start_load = time.perf_counter()

model = SentenceTransformer(
    "sentence-transformers/all-MiniLM-L6-v2"
)

load_time = (time.perf_counter() - start_load) * 1000

print(f"Model loaded in {load_time:.2f} ms")

# Warmup
print("Warming up model...")

for _ in range(5):
    model.encode("natural language processing")

print("Warmup complete.")
print("Worker active and listening...\n")

# Worker loop

while True:

    # Wait for tasks.queue
    if not os.path.exists(QUEUE_FILE):
        time.sleep(POLL_INTERVAL)
        continue

    # Read queue
    queue_start = time.perf_counter()

    try:

        with open(QUEUE_FILE, "r", encoding="utf-8") as f:
            lines = [
                line.strip()
                for line in f
                if line.strip()
            ]

        # Remove queue immediately after reading it
        os.remove(QUEUE_FILE)

    except Exception as e:

        print("Queue read error:", e)
        time.sleep(POLL_INTERVAL)
        continue

    queue_time = (time.perf_counter() - queue_start) * 1000


    # Nothing to process
    if not lines:
        continue

    batch_size = len(lines)
    print(f"\nReceived {batch_size} queries")

    # Generate embeddings

    embedding_start = time.perf_counter()
    embeddings = model.encode(lines, convert_to_numpy=True)
    embedding_time = (time.perf_counter() - embedding_start) * 1000

    # Write embeddings
    write_start = time.perf_counter()
    with open(TEMP_VEC_FILE, "wb") as f:

        f.write(embeddings.astype("float32").tobytes())

        # Make sure data reaches the OS
        f.flush()
        os.fsync(f.fileno())

    # Atomic replacement
    os.replace(TEMP_VEC_FILE, VEC_FILE)

    write_time = (time.perf_counter() - write_start) * 1000

    # Signal C++ that batch is completely finished
    signal_start = time.perf_counter()

    # Create an empty signal file
    with open(DONE_FILE, "w"):
        pass

    signal_time = (time.perf_counter() - signal_start) * 1000

    # Benchmark
    total_python_time = (
        queue_time
        + embedding_time
        + write_time
        + signal_time
    )

    average_time = total_python_time / batch_size


    print("----------------------------------------")
    print("Python Benchmark")
    print("----------------------------------------")

    print(f"Batch size       : {batch_size}")
    print(f"Queue read       : {queue_time:.3f} ms")
    print(f"Embedding        : {embedding_time:.3f} ms")
    print(f"Binary write     : {write_time:.3f} ms")
    print(f"Signal creation  : {signal_time:.3f} ms")
    print(f"Total Python     : {total_python_time:.3f} ms")
    print(f"Average / vector : {average_time:.3f} ms")

    print("----------------------------------------")