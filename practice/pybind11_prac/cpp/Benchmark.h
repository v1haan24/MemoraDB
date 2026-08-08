#pragma once

#include "EmbeddingEngine.h"
using namespace std;

class Benchmark
{
public:
    static void benchmarkModel(const EmbeddingConfig& config, const  vector<string>& text, int runs);
};