#include "st_console.h"
#include "configuration.h"
#include "game_wrapper.h"
#include "st_actor.h"
#include "st_configuration.h"
#include "strength_network.h"
#include <ctime>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace strength {

using namespace minizero;
using namespace minizero::console;
using namespace minizero::utils;

StConsole::StConsole()
{
    RegisterFunction("gogui-analyze_commands", this, &StConsole::cmdGoguiAnalyzeCommands);
    RegisterFunction("strength", this, &StConsole::cmdStrength);
}

void StConsole::initialize()
{
    if (!network_) {
        network_ = std::make_shared<StrengthNetwork>();
        std::dynamic_pointer_cast<StrengthNetwork>(network_)->loadModel(config::nn_file_name, 0);
    }
    if (!actor_) {
        uint64_t tree_node_size = static_cast<uint64_t>(config::actor_num_simulation + 1) * network_->getActionSize();
        actor_ = std::make_shared<StActor>(tree_node_size);
        actor_->setNetwork(network_);
        actor_->reset();
    }
    if (actor_select_action_by_bt) {
        std::string file_name = strength::candidate_sgf_dir;
        std::string cache_file = file_name + ".str";
        bool loaded_from_cache = false;
        size_t strength_count = 0;

        std::ifstream cache_in(cache_file);
        if (cache_in.is_open()) {
            std::string generated_time;
            std::getline(cache_in, generated_time); // skip timestamp line

            std::string values_line;
            if (std::getline(cache_in, values_line)) {
                std::istringstream iss(values_line);
                float value = 0.0f;
                while (iss >> value && strength_count < strength::cand_strength.size()) {
                    strength::cand_strength[strength_count++] = value;
                }
                loaded_from_cache = strength_count > 0;
                std::cerr << "using " << cache_file << " for strengths" << std::endl;
            }
        }

        if (!loaded_from_cache) {
            std::map<int, std::vector<std::pair<float, float>>> candidate_Strength;
            std::vector<EnvironmentLoader> env_loaders_cand;

            std::cerr << "read: " << file_name << std::endl;
            std::vector<EnvironmentLoader> env_loaders_temp = loadGames(file_name);
            env_loaders_cand.insert(env_loaders_cand.end(), env_loaders_temp.begin(), env_loaders_temp.end());

            candidate_Strength = calculatePosStrength(std::vector<EnvironmentLoader>(env_loaders_cand));

            for (auto weighted_strength : candidate_Strength) {
                if (weighted_strength.second.size() > strength_count) { strength_count = weighted_strength.second.size(); }
                for (size_t i = 0; i < weighted_strength.second.size(); i++) {
                    strength::cand_strength[i] = weighted_strength.second[i].first / weighted_strength.second[i].second;
                }
            }

            std::ofstream cache_out(cache_file);
            if (cache_out.is_open()) {
                std::time_t now = std::time(nullptr);
                char time_buf[64];
                if (std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now))) {
                    cache_out << time_buf;
                } else {
                    cache_out << now;
                }
                cache_out << '\n';
                for (size_t i = 0; i < strength_count && i < strength::cand_strength.size(); ++i) {
                    if (i != 0) { cache_out << ' '; }
                    cache_out << strength::cand_strength[i];
                }
                cache_out << '\n';
                std::cerr << "saved to " << cache_file << std::endl;
            }
        }

        for (size_t i = 0; i < strength_count && i < strength::cand_strength.size(); ++i) { std::cerr << strength::cand_strength[i] << " "; }
        std::cerr << std::endl;
    }
    actor_->setNetwork(network_); // for reloading model
}

void StConsole::executeCommandUCI(std::string command)
{
    if (!network_ || !actor_) { initialize(); }
    if (command.back() == '\r') { command.pop_back(); }
    if (command.empty()) { return; }

    // parse command to args
    std::stringstream ss(command);
    std::string tmp;
    std::vector<std::string> args;
    while (std::getline(ss, tmp, ' ')) { args.push_back(tmp); }

    // execute function
    if (function_map_.count(args[0]) == 0) { return errReply(ConsoleResponse::kFail, "Unknown command: " + command); }
    (*function_map_[args[0]])(args);
}

void StConsole::cmdGoguiAnalyzeCommands(const std::vector<std::string>& args)
{
    if (!checkArgument(args, 1, 1)) { return; }
    std::string registered_cmd = "sboard/policy_value/pv\n";
    registered_cmd += "string/strength/strength\n";
    reply(console::ConsoleResponse::kSuccess, registered_cmd);
}

void StConsole::cmdStrength(const std::vector<std::string>& args)
{
    if (!checkArgument(args, 1, 1)) { return; }

    std::ostringstream oss;
    std::shared_ptr<StrengthNetwork> network = std::static_pointer_cast<StrengthNetwork>(network_);
    for (int rotation = 0; rotation < static_cast<int>(utils::Rotation::kRotateSize); ++rotation) { network->pushBack(calculateFeatures(actor_->getEnvironment(), static_cast<utils::Rotation>(rotation))); }
    std::vector<std::shared_ptr<minizero::network::NetworkOutput>> network_outputs = network->forward();
    for (int rotation = 0; rotation < static_cast<int>(utils::Rotation::kRotateSize); ++rotation) {
        std::shared_ptr<StrengthNetworkOutput> network_output = std::static_pointer_cast<StrengthNetworkOutput>(network_outputs[rotation]);
        oss << "rotation: " << utils::getRotationString(static_cast<utils::Rotation>(rotation))
            << ", weight: " << network_output->weight_
            << ", score: " << network_output->score_ << std::endl;
    }

    reply(ConsoleResponse::kSuccess, oss.str());
}

void StConsole::calculatePolicyValue(std::vector<float>& policy, float& value, Rotation rotation /* = Rotation::kRotationNone */)
{
    std::shared_ptr<StrengthNetwork> network = std::static_pointer_cast<StrengthNetwork>(network_);
    int index = network->pushBack(calculateFeatures(actor_->getEnvironment(), rotation));
    std::shared_ptr<StrengthNetworkOutput> network_output = std::static_pointer_cast<StrengthNetworkOutput>(network->forward()[index]);
    value = network_output->value_;
    policy.clear();
    for (size_t action_id = 0; action_id < network_output->policy_.size(); ++action_id) {
        int rotated_id = actor_->getEnvironment().getRotateAction(action_id, rotation);
        policy.push_back(network_output->policy_[rotated_id]);
    }
}
std::map<int, std::vector<std::pair<float, float>>> StConsole::calculatePosStrength(const std::vector<EnvironmentLoader>& env_loaders)
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
std::map<int, std::vector<std::pair<float, float>>> StConsole::calculatePosStrength(const EnvironmentLoader& env_loader)
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
} // namespace strength
