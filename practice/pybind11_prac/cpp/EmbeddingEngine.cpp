#include "EmbeddingEngine.h"
#include <pybind11/stl.h>
using namespace std;

EmbeddingEngine::EmbeddingEngine(const EmbeddingConfig& cfg) : config(cfg)
{
    py::module_ sys = py::module_::import("sys");

    sys.attr("path").attr("insert")(
        0,
        PYTHON_SITE_PACKAGES
    );

    sys.attr("path").attr("insert")(
        0,
        "../python"
    );

    module = py::module_::import("embedding");

    module.attr("load_model")(config.modelName);
}

vector<float> EmbeddingEngine::embed(const string& text)
{
    py::object result = module.attr("embed")(text);
    return result.cast<vector<float>>();
}