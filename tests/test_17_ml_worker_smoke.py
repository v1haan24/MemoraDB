import os
import struct
import subprocess
import sys
import tempfile
import time
from pathlib import Path

VEC_DIM = 384
TABLE = "PythonWorkerSmoke"

ROOT = Path(".")
QUEUE = ROOT / "data" / TABLE / "queue"
TASKS = QUEUE / "tasks.queue"
EMBEDDINGS = QUEUE / "embeddings.vec"
DONE = QUEUE / "done.signal"

# The real worker requires sentence-transformers and may download the model
# the first time it is run.
if not (ROOT / "src" / "semantic" / "ml_worker.py").exists():
    raise AssertionError("src/semantic/ml_worker.py not found")

import shutil
shutil.rmtree(ROOT / "data" / TABLE, ignore_errors=True)
QUEUE.mkdir(parents=True, exist_ok=True)

TASK_LINES = [
    "title: database systems ",
    "title: semantic search ",
    "title: vector embeddings ",
]

with open(TASKS, "w", encoding="utf-8") as f:
    for line in TASK_LINES:
        f.write(line + "\n")

print("[1] Starting the real ml_worker.py...")
proc = subprocess.Popen(
    [sys.executable, "src/semantic/ml_worker.py", TABLE],
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
    text=True,
)

try:
    deadline = time.time() + 180
    while time.time() < deadline:
        if DONE.exists() and EMBEDDINGS.exists():
            break

        # Fail early if the worker process itself exits.
        if proc.poll() is not None:
            output = proc.stdout.read() if proc.stdout else ""
            raise AssertionError(
                "ml_worker.py exited before producing embeddings.\n" + output
            )

        time.sleep(0.05)

    assert DONE.exists(), "Timed out waiting for done.signal"
    assert EMBEDDINGS.exists(), "Timed out waiting for embeddings.vec"

    print("[2] done.signal and embeddings.vec were produced.")

    data = EMBEDDINGS.read_bytes()
    expected_size = len(TASK_LINES) * VEC_DIM * 4
    assert len(data) == expected_size, (
        f"Expected {expected_size} bytes, got {len(data)}"
    )

    values = struct.unpack("<" + "f" * (len(TASK_LINES) * VEC_DIM), data)

    print("[3] Embedding binary size and float layout are valid.")
    assert any(v != 0.0 for v in values)

    print("\n==========================================")
    print("Real ml_worker.py smoke test passed.")
    print("==========================================")

finally:
    if proc.poll() is None:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
