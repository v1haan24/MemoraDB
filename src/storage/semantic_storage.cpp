#include "table.h"

#include "../vector/vecTable.h"
#include "../vector/vector_meta.h"

bool Table::writeEmbedding(const std::string& pk, uint64_t timestamp, const float* embedding) {
    bool hasSemantic = false;
    for (int i = 0; i < meta.columnCount; ++i) hasSemantic = hasSemantic || meta.columns[i].isSemantic;
    if (!hasSemantic) return true;

    vecMeta metadata;
    VectorMeta vectorMeta = metadata.readMetadata(std::string(meta.name) + ".vec");
    if (vectorMeta.payloadSize == 0) return false;
    vecTable vectors(vectorMeta);
    return vectors.insert(pk, timestamp, embedding);
}
