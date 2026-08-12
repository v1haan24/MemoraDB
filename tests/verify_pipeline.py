import os
import struct
import sys

VEC_DIM = 384
TNS = 30
HEADER_SIZE = 4 + TNS + 4 + 4 + 4
PK_SIZE = 16

if len(sys.argv) != 2:
    print("Usage: python verify_pipeline.py <table_name>")
    sys.exit(2)

table = sys.argv[1]
path = os.path.join("data", table, f"{table}.vec")

if not os.path.exists(path):
    raise AssertionError(f"Vector file does not exist: {path}")

with open(path, "rb") as f:
    raw = f.read(HEADER_SIZE)
    assert len(raw) == HEADER_SIZE

    metadata_size = struct.unpack_from("<i", raw, 0)[0]
    name = raw[4:4 + TNS].split(b"\0", 1)[0].decode()
    pk_size = struct.unpack_from("<i", raw, 4 + TNS)[0]
    record_count = struct.unpack_from("<I", raw, 8 + TNS)[0]
    payload_size = struct.unpack_from("<i", raw, 12 + TNS)[0]

    assert metadata_size == HEADER_SIZE
    assert name == "Vector_DB_" + table
    assert pk_size == PK_SIZE
    assert record_count == 3

    expected_payload = 4 + PK_SIZE + 8 + 4 * VEC_DIM
    assert payload_size == expected_payload

    records = []
    for i in range(record_count):
        record = f.read(payload_size)
        assert len(record) == payload_size

        offset = 0
        rec_id = struct.unpack_from("<I", record, offset)[0]
        offset += 4

        pk = record[offset:offset + PK_SIZE].split(b"\0", 1)[0].decode()
        offset += PK_SIZE

        timestamp = struct.unpack_from("<Q", record, offset)[0]
        offset += 8

        embedding = struct.unpack_from(
            "<" + "f" * VEC_DIM, record, offset
        )

        records.append((rec_id, pk, timestamp, embedding))

    expected = [
        (0, "p001", 1001, 1.0),
        (1, "p002", 1002, 2.0),
        (2, "p003", 1003, 3.0),
    ]

    for record, exp in zip(records, expected):
        rec_id, pk, timestamp, embedding = record
        exp_id, exp_pk, exp_ts, exp_first = exp

        assert rec_id == exp_id
        assert pk == exp_pk
        assert timestamp == exp_ts
        assert embedding[0] == exp_first

print("Full C++ -> Python -> .vec pipeline verification passed.")
