#pragma once

#include <filesystem>
#include <string>

namespace sage {

struct PublishConfig {
    std::filesystem::path build_dir{"build"};
    std::filesystem::path dist_dir{"dist"};
    std::string executable_name{"sage_engine"};
    bool create_zip{true};
};

class IPublisher {
public:
    virtual ~IPublisher() = default;
    virtual bool publish(const PublishConfig& config) = 0;
};

} // namespace sage
