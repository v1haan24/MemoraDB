import os
import struct
import time
from sentence_transformers import SentenceTransformer

QUEUE_DIR = os.path.join("data", "embedding_queue")
QUEUE_FILE = os.path.join(QUEUE_DIR, "tasks.queue")
TEMP_VEC_FILE = os.path.join(QUEUE_DIR, "temp_embeddings.vec")
VEC_FILE = os.path.join(QUEUE_DIR, "embeddings.vec")
DONE_FILE = os.path.join(QUEUE_DIR, "done.signal")

POLL_INTERVAL = 0.001  # 1 ms

os.makedirs(QUEUE_DIR, exist_ok=True)

def read_uint32(file):
    raw = file.read(4)
    if len(raw) != 4:
        raise EOFError("Unexpected end of queue while reading uint32")
    return struct.unpack("<I", raw)[0]

def read_uint64(file):
    raw = file.read(8)
    if len(raw) != 8:
        raise EOFError("Unexpected end of queue while reading uint64")
    return struct.unpack("<Q", raw)[0]

def read_string(file):
    length = read_uint32(file)
    raw = file.read(length)
    if len(raw) != length:
        raise EOFError("Unexpected end of queue while reading string")
    return raw.decode("utf-8")

def read_queue(path):
    records = []

    with open(path, "rb") as f:
        count = read_uint32(f)

        for _ in range(count):
            table_name = read_string(f)
            primary_key = read_string(f)
            timestamp = read_uint64(f)
            text = read_string(f)

            records.append((table_name, primary_key, timestamp, text))

    return records


print("========================================")
print("  MemoraDB Global Python ML Worker")
print("========================================")
print(f"Queue directory: {QUEUE_DIR}")
print("One Python worker serves all tables.")
print()
print("Loading all-MiniLM-L6-v2...")

start_load = time.perf_counter()

model = SentenceTransformer("sentence-transformers/all-MiniLM-L6-v2")

load_time = (time.perf_counter() - start_load) * 1000
print(f"Model loaded in {load_time:.2f} ms")

print("Warming up model...")
for _ in range(5):
    model.encode("natural language processing")

print("Warmup complete.")
print("Global worker active and listening...\n")

while True:
    if not os.path.exists(QUEUE_FILE):
        time.sleep(POLL_INTERVAL)
        continue

    queue_start = time.perf_counter()

    try:
        records = read_queue(QUEUE_FILE)

        # Keep tasks.queue until the result has been fully written and the
        # completion signal has been created. If Python crashes before that
        # point, the C++ side can leave the queue available for recovery.

    except Exception as e:
        print("Queue read error:", e)
        time.sleep(POLL_INTERVAL)
        continue

    queue_time = (time.perf_counter() - queue_start) * 1000

    if not records:
        continue

    texts = [record[3] for record in records]
    batch_size = len(texts)

    print(f"\nReceived {batch_size} record(s) from global queue")
    print("Tables:", ", ".join(record[0] for record in records))

    embedding_start = time.perf_counter()

    embeddings = model.encode(texts, convert_to_numpy=True)

    embedding_time = (time.perf_counter() - embedding_start) * 1000

    write_start = time.perf_counter()

    with open(TEMP_VEC_FILE, "wb") as f:
        f.write(embeddings.astype("float32").tobytes())
        f.flush()
        os.fsync(f.fileno())

    os.replace(TEMP_VEC_FILE, VEC_FILE)

    write_time = (time.perf_counter() - write_start) * 1000

    signal_start = time.perf_counter()

    with open(DONE_FILE, "w"):
        pass

    signal_time = (time.perf_counter() - signal_start) * 1000

    # Only remove the input batch after the result and completion signal
    # have both been successfully produced.
    try:
        os.remove(QUEUE_FILE)
    except FileNotFoundError:
        pass

    total_python_time = (queue_time + embedding_time + write_time + signal_time)

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
