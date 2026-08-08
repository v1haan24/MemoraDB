#pragma once

#include <vector>
#include <string>
#include <pybind11/embed.h>
#include <pybind11/stl.h> 
#include "EmbeddingConfig.h"
using namespace std;

namespace py = pybind11;

class EmbeddingEngine
{
public:
    EmbeddingEngine(const EmbeddingConfig& config);

    vector<float> embed(const string& text);

private:
    EmbeddingConfig config;
    py::object module;
};