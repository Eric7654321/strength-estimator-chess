#include "st_mode_handler.h"
#include "evaluator.h"
#include "game_wrapper.h"
#include "git_info.h"
#include "st_actor.h"
#include "st_actor_group.h"
#include "st_configuration.h"
#include "st_console.h"
#include "time_system.h"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace strength {

using namespace minizero;
using namespace minizero::utils;
StModeHandler::StModeHandler()
{
    RegisterFunction("evaluator", this, &StModeHandler::runEvaluator);
    RegisterFunction("mcts_acc", this, &StModeHandler::runMCTSAccuracy);
    // RegisterFunction("run1graphic", this, &StModeHandler::runFullGraphic);
    RegisterFunction("rlc", this, &StModeHandler::runLegalityCheck);
    RegisterFunction("consoleUCI", this, &StModeHandler::runConsoleUCI);
    RegisterFunction("ScoreVar_analysis", this, &StModeHandler::runScoreVarAnalysis);
}
void StModeHandler::loadNetwork(const std::string& nn_file_name, int gpu_id /* = 0 */)
{
    network_ = std::make_shared<StrengthNetwork>();
    network_->loadModel(nn_file_name, gpu_id);
}
void StModeHandler::runConsole()
{
    StConsole console;
    std::string command;
    console.initialize();
    std::cerr << "Successfully started console mode" << std::endl;
    while (getline(std::cin, command)) {
        if (command == "quit") { break; }
        console.executeCommand(command);
    }
}

std::vector<std::string> splitBySpace(const std::string& s)
{
    std::vector<std::string> result;
    std::string current;

    for (char c : s) {
        if (c == ' ') {
            if (!current.empty()) {
                result.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }

    if (!current.empty())
        result.push_back(current);

    return result;
}

void StModeHandler::runConsoleUCI()
{
    StConsole console;

    console.initialize();

    // 關閉 stdout 緩衝，確保 GUI 能即時收到指令
    std::setvbuf(stdout, NULL, _IONBF, 0);

    // Debug 訊息一律用 cerr，不要汙染 UCI 通訊
    std::cerr << "=== MiniZero UCI Interface Initialized (Time Mgmt + Turn Fix) ===" << std::endl;

    std::string input;

    // 0: White, 1: Black. 預設 Startpos 是白先
    // 這個變數會在 position 指令中被校正
    int current_turn = 0;

    while (getline(std::cin, input)) {
        std::vector<std::string> parsed = splitBySpace(input);
        if (parsed.empty()) continue;

        std::string command = parsed[0];

        if (command == "quit") {
            break;
        } else if (command == "uci") {
            std::cout << "id name BongCloudMaster01 (MiniZero)" << std::endl;
            std::cout << "id author Toshi & Dr.Kiwi" << std::endl;

            // 宣告支援的選項 (讓 Lichess 知道這隻 Bot 是正常的)
            std::cout << "option name Hash type spin default 16 min 1 max 1024" << std::endl;
            std::cout << "option name Threads type spin default 4 min 1 max 64" << std::endl;

            std::cout << "uciok" << std::endl;
        } else if (command == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (command == "ucinewgame") {
            console.executeCommandUCI("clear_boardUCI");
            current_turn = 0; // 重置為白先
        } else if (command == "position") {
            // 【狀態同步】每次都清空重擺，這是最穩的做法
            console.executeCommandUCI("clear_boardUCI");
            current_turn = 0;

            size_t moves_index = 0;
            // 尋找 moves 關鍵字
            for (size_t i = 1; i < parsed.size(); ++i) {
                if (parsed[i] == "moves") {
                    moves_index = i + 1;
                    break;
                }
            }

            // 處理 FEN (如果未來支援的話，邏輯放這裡)
            // 目前假設 Lichess 大多送 startpos moves ...

            if (moves_index > 0 && moves_index < parsed.size()) {
                for (size_t i = moves_index; i < parsed.size(); ++i) {
                    std::string move = parsed[i];
                    // 根據當前 turn 決定是 white 還是 black 下這步棋
                    std::string color = (current_turn % 2 == 0) ? "white" : "black";

                    console.executeCommandUCI("play " + color + " " + move);

                    // 換邊
                    current_turn++;
                }
            }
            // 這裡跑完後，current_turn 自動指向「現在該思考的一方」
        } else if (command == "go") {
            // 【時間管理】解析 wtime, btime, winc, binc
            float wtime = 0, btime = 0, winc = 0, binc = 0;
            float movetime = 0; // 支援固定時間模式
            bool infinite = false;

            for (size_t i = 1; i < parsed.size(); ++i) {
                if (parsed[i] == "wtime" && i + 1 < parsed.size())
                    wtime = std::stof(parsed[i + 1]);
                else if (parsed[i] == "btime" && i + 1 < parsed.size())
                    btime = std::stof(parsed[i + 1]);
                else if (parsed[i] == "winc" && i + 1 < parsed.size())
                    winc = std::stof(parsed[i + 1]);
                else if (parsed[i] == "binc" && i + 1 < parsed.size())
                    binc = std::stof(parsed[i + 1]);
                else if (parsed[i] == "movetime" && i + 1 < parsed.size())
                    movetime = std::stof(parsed[i + 1]);
                else if (parsed[i] == "infinite")
                    infinite = true;
            }

            // 判斷是我方剩餘時間
            // 偶數=白方(wtime), 奇數=黑方(btime)
            float my_time = (current_turn % 2 == 0) ? wtime : btime;
            float my_inc = (current_turn % 2 == 0) ? winc : binc;
            float allocated_time_ms = 0;

            if (movetime > 0) {
                // 1. 固定時間模式 (go movetime 5000)
                allocated_time_ms = movetime;
            } else if (!infinite && my_time > 0) {
                // 2. 正常比賽模式 (動態分配)
                // 策略：(剩餘時間 / 20) + (加秒 * 0.8)
                // 這是比較保守且通用的策略
                allocated_time_ms = (my_time / 20.0f) + (my_inc * 0.8f);

                // 上限保護：不要一次花掉超過 80% 的剩餘時間
                if (allocated_time_ms > my_time * 0.8f) allocated_time_ms = my_time * 0.8f;

                // 下限保護：至少給 100ms，不然 MCTS 還沒跑就結束了
                if (allocated_time_ms < 100) allocated_time_ms = 100;
            } else {
                // 3. 無限模式或分析模式 (使用 Config 預設值)
                // 如果是 infinite，通常要等 GUI 送 stop，這裡暫時設大一點
                // 或者維持 config 原本設定
                allocated_time_ms = 0; // 0 代表不覆蓋 config
            }

            // 將計算好的時間套用到 Config (轉成秒)
            if (allocated_time_ms > 0) {
                config::actor_mcts_think_time_limit = allocated_time_ms / 1000.0f;
                std::cerr << "Time Mgmt: Left " << my_time << "ms. Allocating " << allocated_time_ms << "ms." << std::endl;
            }

            // 根據 current_turn 決定 genmove 誰
            std::string cmd_color = (current_turn % 2 == 0) ? "white" : "black";
            std::string genmove_cmd = "genmoveUCI " + cmd_color;

            // 執行思考！
            console.executeCommandUCI(genmove_cmd);

            // 思考完畢，輪次 +1
            current_turn++;
        } else if (command == "stop") {
            // 雖然是 Blocking，但寫著以備未來擴充
            // 這裡可以呼叫 console.stop() 如果有實作的話
        } else if (command == "ponderhit") {
            std::cerr << "I'm lazy, this it not implement yet, ask Toshi to fix it" << std::endl;
            // 預測命中，轉為正式思考 (目前暫時忽略)
        }
    }
}
void StModeHandler::runScoreVarAnalysis()
{
    loadNetwork(config::nn_file_name);

    std::cerr << TimeSystem::getTimeString("[Y/m/d H:i:s.f] ") << "Loading testing sgfs ..." << std::endl;
    std::string file_name = strength::testing_sgf_dir;

    std::cerr << "read: " << file_name << std::endl;
    std::vector<EnvironmentLoader> env_loaders = loadGames(file_name);

    std::cerr << TimeSystem::getTimeString("[Y/m/d H:i:s.f] ") << "Total loaded " << env_loaders.size() << " games" << std::endl;

    for (size_t i = 0; i < env_loaders.size(); ++i) {
        std::cout << "Game " << i << ": " << std::endl;
        const auto& loader = env_loaders[i];

        Environment env;

        std::vector<float> scores;
        for (size_t pos = 0; pos < loader.getActionPairs().size(); ++pos) {
            network_->pushBack(env.getFeatures());
            env.act(loader.getActionPairs()[pos].first);
        }
        env.reset();
        std::vector<std::shared_ptr<network::NetworkOutput>> output = network_->forward();
        for (size_t pos = 0; pos < loader.getActionPairs().size(); ++pos) {
            std::cout << "Position " << pos << ", Action=" << loader.getActionPairs()[pos].first.toConsoleString() << std::endl;
            std::shared_ptr<StrengthNetworkOutput> s_output = std::static_pointer_cast<StrengthNetworkOutput>(output[pos]);
            std::vector<float> policy_output = s_output->policy_;

            std::vector<std::pair<int, float>> action_candidates;
            for (size_t action_id = 0; action_id < policy_output.size(); ++action_id) {
                Action action(action_id, env.getTurn());
                if (!env.isLegalAction(action)) {
                    continue;
                }
                action_candidates.push_back(std::make_pair(action_id, policy_output[action_id]));
            }
            std::sort(action_candidates.begin(), action_candidates.end(),
                      [](const std::pair<int, float>& a, const std::pair<int, float>& b) {
                          return a.second > b.second; // 由大到小排序
                      });
            float sum = 0;
            int count = 0;
            for (size_t j = 0; j < action_candidates.size(); j++) {
                Environment env_copy = env;
                Action action(action_candidates[j].first, env_copy.getTurn());
                env_copy.act(action);

                network_->pushBack(env_copy.getFeatures());
                sum += action_candidates[j].second;
                count++;
                if (sum > 0.9)
                    break;
            }
            std::vector<std::shared_ptr<network::NetworkOutput>> output_ = network_->forward();
            std::vector<float> scores_;

            for (size_t action_ = 0; action_ < output_.size(); action_++) {
                std::shared_ptr<StrengthNetworkOutput> s_output_ = std::static_pointer_cast<StrengthNetworkOutput>(output_[action_]);
                std::cout << Action(action_candidates[action_].first, env.getTurn()).toConsoleString() << ":" << action_candidates[action_].second << ", " << s_output_->score_ << std::endl;
                scores_.push_back(s_output_->score_);
            }

            // calculate variance
            float mean = 0.0f;
            for (auto s : scores_) { mean += s; }
            mean /= scores_.size();
            float var = 0.0f;
            for (auto s : scores_) { var += (s - mean) * (s - mean); }
            var /= scores_.size();
            std::cout << "var=" << var << std::endl;
            env.act(loader.getActionPairs()[pos].first);
        }
        std::cout << std::endl;
    }
}
void StModeHandler::runSelfPlay()
{
    STActorGroup ag;
    ag.run();
}

void StModeHandler::runZeroTrainingName()
{
    std::cout << Environment().name()                  // name for environment
              << "_" << getNetworkAbbeviation()        // network & training algorithm
              << "_" << config::nn_num_blocks << "b"   // number of blocks
              << "x" << config::nn_num_hidden_channels // number of hidden channels
              << "-" << GIT_SHORT_HASH << std::endl;   // git hash info
}

void StModeHandler::runEvaluator()
{
    Evaluator evaluator;
    evaluator.run();
}
void StModeHandler::runMCTSAccuracy()
{
    if (actor_select_action_by_bt) {
        loadNetwork(config::nn_file_name);
        std::string file_name;
        std::map<int, std::vector<std::pair<float, float>>> candidate_Strength;

        std::vector<EnvironmentLoader> env_loaders_cand;

        file_name = strength::candidate_sgf_dir;

        std::cerr << "read: " << file_name << std::endl;
        std::vector<EnvironmentLoader> env_loaders_temp = loadGames(file_name);
        env_loaders_cand.insert(env_loaders_cand.end(), env_loaders_temp.begin(), env_loaders_temp.end());

        candidate_Strength = calculatePosStrength(std::vector<EnvironmentLoader>(env_loaders_cand));

        for (auto weighted_strength : candidate_Strength) {
            for (size_t i = 0; i < weighted_strength.second.size(); i++) {
                strength::cand_strength[i] = weighted_strength.second[i].first / weighted_strength.second[i].second;
                std::cerr << strength::cand_strength[i] << " ";
            }
        }
        std::cerr << std::endl;
    }

    std::cerr << TimeSystem::getTimeString("[Y/m/d H:i:s.f] ") << "Loading testing sgfs ..." << std::endl;
    std::string file_name = strength::testing_sgf_dir;

    std::cerr << "read: " << file_name << std::endl;
    std::vector<EnvironmentLoader> env_loaders = loadGames(file_name);

    std::cerr << TimeSystem::getTimeString("[Y/m/d H:i:s.f] ") << "Total loaded " << env_loaders.size() << " games" << std::endl;

    std::cerr << TimeSystem::getTimeString("[Y/m/d H:i:s.f] ") << "Running MCTS accuracy ..." << std::endl;
    STActorGroup ag;
    ag.initialize();
    std::vector<int> game_index(ag.getActors().size(), -1);
    std::vector<std::shared_ptr<actor::BaseActor>>& actors = ag.getActors();
    bool is_done = false;
    int current_game_index = 0;

    std::vector<int> mcts_correct(config::actor_num_simulation, 0);
    std::vector<std::vector<int>> ssa_correct_(temp_for_mcts_ssa_accuracy.size(), std::vector<int>(config::actor_num_simulation, 0));

    std::vector<int> total(config::actor_num_simulation, 0);
    while (!is_done) {
        is_done = true;
        for (size_t i = 0; i < actors.size(); ++i) {
            int move_number = actors[i]->getEnvironment().getActionHistory().size();
            if (game_index[i] != -1 && move_number < static_cast<int>(env_loaders[game_index[i]].getActionPairs().size())) {
                actors[i]->reset();
                is_done = false;
                for (int j = 0; j < move_number; ++j) { actors[i]->act(env_loaders[game_index[i]].getActionPairs()[j].first); }

            } else if (current_game_index < static_cast<int>(env_loaders.size())) {
                is_done = false;
                actors[i]->reset();
                game_index[i] = current_game_index++;
            } else {
                game_index[i] = -1;
                actors[i]->reset();
            }
        }
        if (is_done) { break; }
        ag.step();
        for (size_t i = 0; i < actors.size(); ++i) {
            if (game_index[i] == -1) { continue; }
            std::shared_ptr<StActor> actor = std::static_pointer_cast<StActor>(actors[i]);
            for (size_t j = 0; j < actor->getMCTSActionPerSimulation().size(); ++j) {
                const Action& mcts_action = actor->getMCTSActionPerSimulation()[j];
                const Action& sgf_action = env_loaders[game_index[i]].getActionPairs()[actors[i]->getEnvironment().getActionHistory().size() - 1].first;
                if (mcts_action.getActionID() == sgf_action.getActionID()) { mcts_correct[j]++; }
                for (size_t k = 0; k < temp_for_mcts_ssa_accuracy.size(); k++) {
                    const Action& ssa_action_ = actor->getSSAActionPerSimulation()[k][j];
                    if (ssa_action_.getActionID() == sgf_action.getActionID()) { ssa_correct_[k][j]++; }
                }
                total[j]++;
            }
        }
        // summary
        for (size_t i = 0; i < total.size(); ++i) {
            std::cout << "simulation: " << (i + 1)
                      << ", mcts accuracy: " << (mcts_correct[i] * 100.0 / total[i]) << "% (" << mcts_correct[i] << "/" << total[i] << ")";

            for (size_t j = 0; j < temp_for_mcts_ssa_accuracy.size(); j++) {
                std::cout << ", ssa_" << temp_for_mcts_ssa_accuracy[j] << " accuracy: " << (ssa_correct_[j][i] * 100.0 / total[i]) << "% (" << ssa_correct_[j][i] << "/" << total[i] << ")";
            }
            std::cout << std::endl;
        }
    }
    exit(0);
}

// void StModeHandler::runFullGraphic()
// {
//     std::cerr << TimeSystem::getTimeString("[Y/m/d H:i:s.f] ")
//               << "Loading training sgfs ..." << std::endl;

//     std::string file_name = strength::training_sgf_dir;

//     std::cerr << "read: " << file_name << std::endl;
//     std::vector<EnvironmentLoader> env_loaders = loadGames(file_name);

//     std::cerr << TimeSystem::getTimeString("[Y/m/d H:i:s.f] ")
//               << "Total loaded " << env_loaders.size() << " games" << std::endl;

//     // ----------------------------------------------------
//     // Replay each game and verify action legality
//     // ----------------------------------------------------
//     const auto& loader = env_loaders[0];
//     const auto& action_pairs = loader.getActionPairs();

//     Environment env;
//     env.reset();

//     for (size_t m = 0; m < action_pairs.size(); ++m) {
//         const Action& action = action_pairs[m].first;

//         // ---- Apply action ----
//         env.act(action);
//         std::cerr << "move " << m + 1 << " state: " << env.getFen() << std::endl;
//     }
//     exit(0);
// }

void StModeHandler::runLegalityCheck()
{
    std::cerr << TimeSystem::getTimeString("[Y/m/d H:i:s.f] ")
              << "Loading training sgfs ..." << std::endl;

    std::string file_name = strength::training_sgf_dir;

    std::cerr << "read: " << file_name << std::endl;
    std::vector<EnvironmentLoader> env_loaders = loadGames(file_name);

    std::cerr << TimeSystem::getTimeString("[Y/m/d H:i:s.f] ")
              << "Total loaded " << env_loaders.size() << " games" << std::endl;

    // ----------------------------------------------------
    // Replay each game and verify action legality
    // ----------------------------------------------------
    struct GameLegalityResult {
        int game_index;
        int total_moves;
        int illegal_moves;
        std::map<int, std::string> illegal_move_indices;
    };

    std::vector<GameLegalityResult> results;
    results.reserve(env_loaders.size());

    for (size_t g = 0; g < env_loaders.size(); ++g) {
        const auto& loader = env_loaders[g];
        const auto& action_pairs = loader.getActionPairs();

        Environment env;
        env.reset();

        GameLegalityResult res;
        res.game_index = static_cast<int>(g);
        res.total_moves = static_cast<int>(action_pairs.size());
        res.illegal_moves = 0;

        for (size_t m = 0; m < action_pairs.size(); ++m) {
            const Action& action = action_pairs[m].first;

            // ---- Check legality at this step ----
            if (!env.isLegalAction(action)) {
                res.illegal_moves++;
                res.illegal_move_indices[static_cast<int>(m)] = action.toConsoleString();
            }

            // ---- Apply action ----
            env.act(action);
        }

        results.push_back(res);
    }

    // ----------------------------------------------------
    // Print summary
    // ----------------------------------------------------
    std::cerr << "========== Testing Dataset Legality Check ==========\n";

    int total_illegal_games = 0;

    for (const auto& r : results) {
        if (r.illegal_moves > 0) total_illegal_games++;

        std::cerr << "Game #" << r.game_index
                  << " | moves: " << r.total_moves
                  << " | illegal: " << r.illegal_moves;

        if (!r.illegal_move_indices.empty()) {
            std::cerr << " | at moves: ";
            for (auto idx : r.illegal_move_indices)
                std::cerr << idx.first << " " << idx.second << ",";
        }

        std::cerr << std::endl;
    }

    std::cerr << "----------------------------------------------------\n";
    std::cerr << "Games with illegal moves: " << total_illegal_games << " / "
              << results.size() << std::endl;

    exit(0);
}

std::map<int, std::vector<std::pair<float, float>>> StModeHandler::calculatePosStrength(const std::vector<EnvironmentLoader>& env_loaders)
{
    if (env_loaders.empty()) { return {}; }
    std::vector<std::pair<float, float>> init(400, {0.0f, 0.0f});
    std::map<int, std::vector<std::pair<float, float>>> results;
    for (size_t i = 0; i < env_loaders.size(); ++i) {
        std::map<int, std::vector<std::pair<float, float>>> tmp = calculatePosStrength(env_loaders[i]);

        for (auto j : tmp) {
            if (results[j.first].size() == 0) results[j.first] = init;
            for (size_t k = 0; k < j.second.size(); k++) {
                results[j.first][k].first += j.second[k].first;
                results[j.first][k].second += j.second[k].second;
            }
        }
    }
    return results;
}
std::map<int, std::vector<std::pair<float, float>>> StModeHandler::calculatePosStrength(const EnvironmentLoader& env_loader)
{
    std::shared_ptr<StrengthNetwork> network = std::static_pointer_cast<StrengthNetwork>(network_);
    int count = 0;
    std::map<int, std::vector<std::pair<float, float>>> results;
    for (size_t pos = 0; pos < env_loader.getActionPairs().size(); ++pos) {
        Rotation rotation = static_cast<Rotation>(Random::randInt() % static_cast<int>(Rotation::kRotateSize));
        std::vector<float> features = calculateFeatures(env_loader, pos, rotation);
        network->pushBack(features);
        count++;
        if ((pos + 1) % 100 == 0 || pos == env_loader.getActionPairs().size() - 1) {
            std::vector<std::shared_ptr<network::NetworkOutput>> output = network->forward();
            for (int pos_ = 0; pos_ < count; ++pos_) {
                std::shared_ptr<StrengthNetworkOutput> s_output = std::static_pointer_cast<StrengthNetworkOutput>(output[pos_]);
                int rank = getRank(env_loader);
                if (strength::bt_use_weight) {
                    results[rank].push_back(std::make_pair(s_output->score_ * s_output->weight_, s_output->weight_));
                } else {
                    results[rank].push_back(std::make_pair(s_output->score_, 1));
                }
            }
            count = 0;
        }
    }
    return results;
}
std::string StModeHandler::getNetworkAbbeviation() const
{
    if (config::nn_type_name == "alphazero") {
        return "az";
    } else if (config::nn_type_name == "bt") {
        return "bt_b" + std::to_string(strength::bt_num_batch_size) + "_r" + std::to_string(strength::bt_num_rank_per_batch) + "_p" + std::to_string(strength::bt_num_position_per_rank);
    } else {
        return config::nn_type_name;
    }
}

} // namespace strength
