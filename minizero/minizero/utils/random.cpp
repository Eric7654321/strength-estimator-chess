#include "random.h"

namespace minizero::utils {

thread_local std::mt19937 Random::generator_;
thread_local std::uniform_int_distribution<int> Random::int_distribution_;
thread_local std::uniform_int_distribution<uint64_t> Random::uint64_distribution_;
thread_local std::uniform_real_distribution<double> Random::real_distribution_;

} // namespace minizero::utils
