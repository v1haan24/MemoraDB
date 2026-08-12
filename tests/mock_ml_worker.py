import os
import struct
import sys
import time

VEC_DIM = 384
POLL_INTERVAL = 0.005

if len(sys.argv) != 2:
    print("Usage: python mock_ml_worker.py <table_name>")
    sys.exit(2)

table = sys.argv[1]
queue_dir = os.path.join("data", table, "queue")

queue_file = os.path.join(queue_dir, "tasks.queue")
temp_vec = os.path.join(queue_dir, "tempEmbed.vec")
vec_file = os.path.join(queue_dir, "embeddings.vec")
done_file = os.path.join(queue_dir, "done.signal")

os.makedirs(queue_dir, exist_ok=True)

print(f"Mock Python worker active for {table}", flush=True)

while True:
    if not os.path.exists(queue_file):
        time.sleep(POLL_INTERVAL)
        continue

    with open(queue_file, "r", encoding="utf-8") as f:
        lines = [line.rstrip("\n") for line in f if line.strip()]

    os.remove(queue_file)

    if not lines:
        continue

    # Deterministic fake embeddings. The first value encodes the row index;
    # all remaining dimensions are deterministic as well.
    values = []
    for i, _ in enumerate(lines):
        values.extend(
            [float(i + 1)] +
            [float((i + 1) * 0.001 + j * 0.00001)
             for j in range(1, VEC_DIM)]
        )

    with open(temp_vec, "wb") as f:
        f.write(struct.pack("<" + "f" * len(values), *values))
        f.flush()
        os.fsync(f.fileno())

    os.replace(temp_vec, vec_file)

    with open(done_file, "w", encoding="utf-8"):
        pass

    print(f"Mock worker processed {len(lines)} record(s)", flush=True)
