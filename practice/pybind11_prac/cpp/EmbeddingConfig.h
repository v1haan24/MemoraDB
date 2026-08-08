// This file is not used completely but I have defined it for testing on cpu as well as gpu for future purpose
#pragma once

#include <string>
using namespace std;

struct EmbeddingConfig
{
    string modelName =
        "sentence-transformers/all-MiniLM-L6-v2";

    string device = "cpu";

    bool normalize = false;
};