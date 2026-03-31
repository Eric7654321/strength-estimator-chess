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
    // if (strength::actor_select_action_by_bt) {
    //     std::cerr << "[UCI] Loading Strength Network..." << std::endl;
    //     loadNetwork(config::nn_file_name);
        
    //     std::string file_name = strength::candidate_sgf_dir;
    //     std::cerr << "[UCI] Loading Candidate Games: " << file_name << std::endl;

    //     std::vector<EnvironmentLoader> env_loaders_cand = loadGames(file_name);
        
    //     std::cerr << "[UCI] Games loaded (" << env_loaders_cand.size() << "), calculating strength..." << std::endl;

    //     auto candidate_Strength = calculatePosStrength(env_loaders_cand);

    //     std::cerr << "[UCI] Strength calculated." << std::endl;

    //     for (auto weighted_strength : candidate_Strength) {
    //         for (size_t i = 0; i < weighted_strength.second.size(); i++) {
    //             strength::cand_strength[i] = weighted_strength.second[i].first / weighted_strength.second[i].second;
    //             // std::cerr << strength::cand_strength[i] << " "; // 註解掉以免太長
    //         }
    //     }
    //     std::cerr << "[UCI] Strength Init Complete!" << std::endl;
        
    //     static std::vector<EnvironmentLoader> keep_alive = std::move(env_loaders_cand);
    // } else {
    //     // 如果沒開 BT，也要載入 Base Model (AlphaZero)
    // }

    StConsole console;

    console.initialize();

    std::setvbuf(stdout, NULL, _IONBF, 0);

    std::cerr << "=== MiniZero UCI Interface Initialized (Time Mgmt + Turn Fix) ===" << std::endl;

    std::string input;
    
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

            std::cout << "option name Hash type spin default 16 min 1 max 1024" << std::endl;
            std::cout << "option name Threads type spin default 4 min 1 max 64" << std::endl;

            std::cout << "uciok" << std::endl;
        } else if (command == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (command == "ucinewgame") {
            console.executeCommandUCI("clear_boardUCI");
            current_turn = 0;
        } else if (command == "position") {
            console.executeCommandUCI("clear_boardUCI");
            current_turn = 0;

            size_t moves_index = 0;
            for (size_t i = 1; i < parsed.size(); ++i) {
                if (parsed[i] == "moves") {
                    moves_index = i + 1;
                    break;
                }
            }

            if (moves_index > 0 && moves_index < parsed.size()) {
                for (size_t i = moves_index; i < parsed.size(); ++i) {
                    std::string move = parsed[i];
                    std::string color = (current_turn % 2 == 0) ? "white" : "black";

                    console.executeCommandUCI("play " + color + " " + move);

                    current_turn++;
                }
            }
        } else if (command == "go") {
            float wtime = 0, btime = 0, winc = 0, binc = 0;
            float movetime = 0;
            bool infinite = false;

            // 1. 解析 UCI 參數
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

            float allocated_time_ms = 15000.0f; // 固定 15 秒

            if (movetime > 0) {
                allocated_time_ms = movetime;
            } else if (infinite) {
                allocated_time_ms = 0;
            } else {
                allocated_time_ms = 15000.0f; 
            }

            // 防被拍死
            float my_time = (current_turn % 2 == 0) ? wtime : btime;
            if (my_time > 0 && allocated_time_ms > my_time - 500.0f) {
                allocated_time_ms = (my_time > 500.0f) ? (my_time - 500.0f) : 100.0f;
            }

            if (allocated_time_ms > 0) {
                config::actor_mcts_think_time_limit = allocated_time_ms / 1000.0f;
            } else {
                config::actor_mcts_think_time_limit = 0.0f; 
            }

            std::string cmd_color = (current_turn % 2 == 0) ? "white" : "black";
            std::string genmove_cmd = "genmoveUCI " + cmd_color;

            console.executeCommandUCI(genmove_cmd);
            current_turn++;
        } else if (command == "stop") {
            // nothing
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