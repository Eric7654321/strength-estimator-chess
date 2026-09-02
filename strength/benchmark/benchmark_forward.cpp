// benchmark_forward.cpp
//
// 目的：量出「純 model forward」在 batch=1 時的真實耗時，
// 排除 MCTS tree 操作、selection、feature 計算等 CPU 端邏輯的干擾。
//
// 用法：
//   ./benchmark_forward <model.pt> <gpu_id> [num_iterations]
// 例如：
//   ./benchmark_forward ./model/new_se_infty.pt 0 1000
//
// 編譯方式請參考你們現有的 CMakeLists.txt，把這個檔案加進去當一個獨立
// executable target，連結到跟 strength_chess 一樣的 libtorch / network 相關
// library 即可（需要連結到 create_network.h 所依賴的東西，或直接連結
// alphazero_network.h / network.cpp 編譯出來的 object）。

#include "network.h"
#include "strength_network.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

using namespace minizero::network;
using namespace strength;

int main(int argc, char* argv[])
{
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <model.pt> <gpu_id> [num_iterations]" << std::endl;
        return 1;
    }

    std::string model_path = argv[1];
    int gpu_id = std::stoi(argv[2]);
    int num_iterations = (argc >= 4 ? std::stoi(argv[3]) : 1000);
    const int num_warmup = 30;

    std::cerr << "Loading model: " << model_path << " on GPU " << gpu_id << std::endl;

    // 注意：你的引擎用的是 bt 型網路（StrengthNetwork），不是泛用的
    // createNetwork()。createNetwork() 對 "bt" type 沒有處理分支，在 release
    // build (NDEBUG，assert 被關閉) 會直接回傳 nullptr，造成 segfault。
    // 改成跟 st_console.cpp 完全一樣的 load 方式：
    std::shared_ptr<StrengthNetwork> network = std::make_shared<StrengthNetwork>();
    network->loadModel(model_path, gpu_id);
    std::cerr << network->toString() << std::endl;

    int num_input_channels = network->getNumInputChannels();
    int height = network->getInputChannelHeight();
    int width = network->getInputChannelWidth();
    int feature_size = num_input_channels * height * width;

    std::cerr << "Input shape: [" << num_input_channels << ", " << height << ", " << width
              << "] (feature_size=" << feature_size << ")" << std::endl;

    // 準備一筆假資料，數值不重要（隨機即可），重點是 shape 要跟真實 inference 一致
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::vector<float> dummy_features(feature_size);
    for (auto& v : dummy_features) v = dist(rng);

    // ---------- Warmup ----------
    // 第一次 forward 通常較慢（cuDNN algorithm search、CUDA context 真正初始化等），
    // 不 warmup 會把這個一次性成本算進平均值，誤導判斷穩態效能。
    std::cerr << "Warming up (" << num_warmup << " iterations)..." << std::endl;
    for (int i = 0; i < num_warmup; ++i) {
        network->pushBack(dummy_features);
        network->forward();
    }

    // ---------- Benchmark: 純 forward (batch=1) ----------
    std::cerr << "Benchmarking " << num_iterations << " forwards at batch_size=1..." << std::endl;

    std::vector<double> per_call_ms;
    per_call_ms.reserve(num_iterations);

    auto total_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_iterations; ++i) {
        auto t0 = std::chrono::high_resolution_clock::now();

        network->pushBack(dummy_features);
        auto outputs = network->forward();
        // outputs 變數刻意保留，避免編譯器把整段呼叫優化掉
        volatile float touch = outputs.empty() ? 0.0f : 1.0f;
        (void)touch;

        auto t1 = std::chrono::high_resolution_clock::now();
        per_call_ms.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    auto total_end = std::chrono::high_resolution_clock::now();

    double total_ms = std::chrono::duration<double, std::milli>(total_end - total_start).count();
    double avg_ms = total_ms / num_iterations;

    // 算一下分布，不只看平均值——如果 stddev 很大，代表有些次特別慢（例如
    // 被其他 process 搶資源），平均值會誤導判斷。
    double sum = std::accumulate(per_call_ms.begin(), per_call_ms.end(), 0.0);
    double mean = sum / per_call_ms.size();
    double sq_sum = 0.0;
    for (double v : per_call_ms) sq_sum += (v - mean) * (v - mean);
    double stddev = std::sqrt(sq_sum / per_call_ms.size());

    std::sort(per_call_ms.begin(), per_call_ms.end());
    double p50 = per_call_ms[per_call_ms.size() * 0.50];
    double p90 = per_call_ms[per_call_ms.size() * 0.90];
    double p99 = per_call_ms[std::min(per_call_ms.size() - 1, (size_t)(per_call_ms.size() * 0.99))];
    double min_ms = per_call_ms.front();
    double max_ms = per_call_ms.back();

    std::cout << "==================== RESULT ====================" << std::endl;
    std::cout << "Total time:      " << total_ms << " ms over " << num_iterations << " forwards" << std::endl;
    std::cout << "Avg per forward: " << avg_ms << " ms" << std::endl;
    std::cout << "Stddev:          " << stddev << " ms" << std::endl;
    std::cout << "Min / Max:       " << min_ms << " / " << max_ms << " ms" << std::endl;
    std::cout << "P50 / P90 / P99: " << p50 << " / " << p90 << " / " << p99 << " ms" << std::endl;
    std::cout << "=================================================" << std::endl;
    std::cout << std::endl;
    std::cout << "對照：你的正式引擎 500 simulation 約 8~9 秒，等於每個 simulation ~16-18ms。" << std::endl;
    std::cout << "若這裡測出的 avg 接近 16ms -> 瓶頸在 model forward 本身，TensorRT/FP16 值得投入。" << std::endl;
    std::cout << "若這裡測出的 avg 遠小於 16ms (例如 <8ms) -> 瓶頸在 MCTS/feature 計算等 CPU 端邏輯，TensorRT 救不了。" << std::endl;

    return 0;
}