from sentence_transformers import SentenceTransformer
import time

model = None


def load_model(model_name):
    global model

    print(f"Loading model: {model_name}")

    t0 = time.perf_counter()
    model = SentenceTransformer(model_name)
    t1 = time.perf_counter()

    print(f"Model loaded in {(t1 - t0) * 1000:.2f} ms")


def embed(text):
    if model is None:
        raise RuntimeError("Model has not been loaded.")

    t0 = time.perf_counter()
    embedding = model.encode(text)
    t1 = time.perf_counter()

    print(f"Embedding generated in {(t1 - t0) * 1000:.2f} ms")

    return embedding.tolist()