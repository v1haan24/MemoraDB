#include "Benchmark.h"
#include <vector>
#include <string>
using namespace std;


int main()
{
    py::scoped_interpreter guard{}; //Create the python interpreter only once
    vector<string> models =
    {
        "sentence-transformers/all-MiniLM-L6-v2",
        "sentence-transformers/static-similarity-mrl-multilingual-v1",
        //"BAAI/bge-small-en-v1.5",
        //"intfloat/e5-small-v2"
    };

    // const string text =
    //     "Apple is a fruit.";

    const vector<string> texts =
    {
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis",
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis",
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis",
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis",
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis",
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis",
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis",
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis",
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis",
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis",
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis",
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis",
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis",
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis",
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis",
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis",
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis",
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis",
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis",
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis",
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis",
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis",
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis",
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis",
        "natural language processing",
        "semantic search engine",
        "vector database indexing",
        "Python embedding models analysis"
    };


    const int runs = 100;

    for(const auto& model : models)
    {
        EmbeddingConfig config;
        config.modelName = model;

        Benchmark::benchmarkModel(config, texts, runs);
    }

    return 0;
}