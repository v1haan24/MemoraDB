#include "Benchmark.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>

using namespace std;
using namespace chrono;

void Benchmark::benchmarkModel(const EmbeddingConfig& config, const vector<string>& text, int runs)
{
    cout << "\n=========================================\n";
    cout << "Model : " << config.modelName << '\n';
    cout << "=========================================\n\n";

    //-------------------------------------------------------
    // Measure model loading time
    //-------------------------------------------------------

    auto loadStart = high_resolution_clock::now();

    EmbeddingEngine engine(config);

    auto loadEnd = high_resolution_clock::now();

    double loadTime =
        duration<double, milli>(loadEnd - loadStart).count();

    cout << fixed << setprecision(2);
    cout << "Model Load Time : "<< loadTime<< " ms\n\n";

    //Warmup Embedding as 1st embed takes the most time
    auto embed = engine.embed("Warmup");

    //-------------------------------------------------------
    // Measure encoding times
    //-------------------------------------------------------

    double totalTime = 0.0;

    double minTime = numeric_limits<double>::max();
    double maxTime = 0.0;

    int dimension = 0;

    for(int i = 0; i < runs; i++)
    {
        auto start = high_resolution_clock::now();

        auto embedding = engine.embed(text[i]);

        auto end = high_resolution_clock::now();

        double elapsed = duration<double, milli>(end - start).count();

        totalTime += elapsed;

        minTime = min(minTime, elapsed);
        maxTime = max(maxTime, elapsed);

        dimension = embedding.size();

        cout<< "Run "<< setw(3)<< i + 1<< " : "<< elapsed << " ms\n";
    }

    //-------------------------------------------------------
    // Summary
    //-------------------------------------------------------

    cout << "\n----------- Summary -----------\n";

    cout << "Model            : "<< config.modelName<< '\n';


    cout << "Dimension        : "<< dimension<< '\n';

    cout << "Total time       : "<< totalTime<< " ms for "<<text.size()<<" items\n";

    cout << "Average Time     : "<< totalTime / runs<< " ms\n";

    cout << "Minimum Time     : "<< minTime << " ms\n";

    cout << "Maximum Time     : "<< maxTime<< " ms\n";

    cout << "Model Load Time  : "<< loadTime<< " ms\n";
}