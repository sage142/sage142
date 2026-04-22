#include "sage/modules.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace sage {
namespace fs = std::filesystem;

class Publisher final : public IPublisher {
public:
    bool publish(const PublishConfig& config) override {
        const std::string platform = detect_platform();
        const fs::path staging_dir = config.dist_dir / platform;
        const fs::path executable_src = config.build_dir / config.executable_name;
        const fs::path executable_dst = staging_dir / config.executable_name;

        std::error_code ec;
        fs::create_directories(staging_dir, ec);
        if (ec) {
            std::cerr << "Failed to create staging directory: " << ec.message() << "\n";
            return false;
        }

        fs::copy_file(executable_src, executable_dst, fs::copy_options::overwrite_existing, ec);
        if (ec) {
            std::cerr << "Failed to copy executable from " << executable_src << ": " << ec.message() << "\n";
            return false;
        }

        const fs::path assets_src = "assets";
        const fs::path assets_dst = staging_dir / "assets";
        fs::create_directories(assets_dst, ec);
        fs::copy(assets_src, assets_dst,
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing,
                 ec);
        if (ec) {
            std::cerr << "Failed to copy assets: " << ec.message() << "\n";
            return false;
        }

        if (config.create_zip) {
            const fs::path zip_path = config.dist_dir / (platform + ".zip");
            std::stringstream cmd;
            cmd << "cd \"" << config.dist_dir.string() << "\" && zip -r \""
                << zip_path.filename().string() << "\" \"" << platform << "\" > /dev/null";
            const int zip_exit = std::system(cmd.str().c_str());
            if (zip_exit != 0) {
                std::cerr << "Archive step skipped/failed (zip tool may be missing).\n";
            }
        }

        std::cout << "Publish complete: " << staging_dir << "\n";
        return true;
    }

private:
    static std::string detect_platform() {
#if defined(_WIN32)
        return "windows";
#elif defined(__APPLE__)
        return "macos";
#else
        return "linux";
#endif
    }
};

std::unique_ptr<IPublisher> make_publisher() {
    return std::make_unique<Publisher>();
}

} // namespace sage
