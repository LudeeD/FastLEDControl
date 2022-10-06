#include "utils.hpp"

namespace teton {
namespace utils {

bool getEnvVar(std::string env, std::string &result) {
    if (const char *tmpResult = std::getenv(env.c_str())) {
        result = tmpResult;
        return true;
    }

    return false;
}

}  // namespace utils
}  // namespace teton
