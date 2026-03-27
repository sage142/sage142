#include <algorithm>
#include <chrono>
#include <cctype>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <vector>

enum class TaskType {
    Work,
    Chat,
    Game,
    Unknown
};

struct TaskPlan {
    TaskType type{TaskType::Unknown};
    std::string app;
    std::vector<std::string> actions;
};

class SafetyGate {
public:
    static bool confirmExecution(const TaskPlan &plan) {
        std::cout << "\nProposed task for app: " << plan.app << "\n";
        for (const auto &action : plan.actions) {
            std::cout << "  - " << action << '\n';
        }
        std::cout << "\nRun these actions? (yes/no): ";
        std::string answer;
        std::getline(std::cin, answer);
        normalize(answer);
        return answer == "yes" || answer == "y";
    }

private:
    static void normalize(std::string &s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) {
            return std::isspace(c);
        }), s.end());
    }
};

class DesktopController {
public:
    void run(const TaskPlan &plan) {
        std::cout << "\n[Controller] Starting automation sequence...\n";

        for (const auto &action : plan.actions) {
            std::cout << "[Controller] " << action << '\n';
            std::this_thread::sleep_for(std::chrono::milliseconds(350));
        }

        std::cout << "[Controller] Done.\n\n";
    }
};

class TinyAssistantBrain {
public:
    std::optional<TaskPlan> buildPlan(std::string input) {
        normalize(input);
        if (input.empty()) {
            return std::nullopt;
        }

        TaskPlan plan;
        if (contains(input, "chat") || contains(input, "message") || contains(input, "reply")) {
            plan.type = TaskType::Chat;
            plan.app = detectApp(input, {"slack", "discord", "teams", "telegram", "whatsapp"}, "chat app");
            plan.actions = {
                "Focus the chat window",
                "Open unread conversations",
                "Draft suggested reply",
                "Wait for user confirmation before sending"
            };
            return plan;
        }

        if (contains(input, "work") || contains(input, "email") || contains(input, "spreadsheet")) {
            plan.type = TaskType::Work;
            plan.app = detectApp(input, {"excel", "sheets", "outlook", "gmail", "notion"}, "work app");
            plan.actions = {
                "Switch to the work window",
                "Read current context",
                "Perform requested edits or summary",
                "Save draft changes"
            };
            return plan;
        }

        if (contains(input, "game") || contains(input, "grind") || contains(input, "farm")) {
            plan.type = TaskType::Game;
            plan.app = detectApp(input, {"minecraft", "fortnite", "valorant", "steam"}, "game");
            plan.actions = {
                "Activate game window",
                "Run low-risk repetitive sequence",
                "Pause if unexpected popup appears",
                "Hand control back every 60 seconds"
            };
            return plan;
        }

        plan.type = TaskType::Unknown;
        plan.app = "desktop";
        plan.actions = {
            "Parse the instruction",
            "Ask follow-up question",
            "Perform one safe action at a time"
        };
        return plan;
    }

private:
    static bool contains(const std::string &text, const std::string &token) {
        return text.find(token) != std::string::npos;
    }

    static std::string detectApp(const std::string &input,
                                 const std::vector<std::string> &apps,
                                 const std::string &fallback) {
        for (const auto &app : apps) {
            if (contains(input, app)) {
                return app;
            }
        }
        return fallback;
    }

    static void normalize(std::string &text) {
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
    }
};

int main() {
    std::cout << "ScreenPilot (C++ AI screen controller prototype)\n";
    std::cout << "Type a task (examples: \"reply on slack\", \"work on spreadsheet\", \"game farming\").\n";
    std::cout << "Type \"quit\" to exit.\n\n";

    TinyAssistantBrain brain;
    DesktopController controller;

    while (true) {
        std::cout << "> ";
        std::string input;
        std::getline(std::cin, input);
        if (!std::cin) {
            break;
        }

        std::string lowered = input;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        if (lowered == "quit" || lowered == "exit") {
            std::cout << "Goodbye.\n";
            break;
        }

        auto maybePlan = brain.buildPlan(input);
        if (!maybePlan.has_value()) {
            std::cout << "Please enter a non-empty task.\n";
            continue;
        }

        if (SafetyGate::confirmExecution(maybePlan.value())) {
            controller.run(maybePlan.value());
        } else {
            std::cout << "Canceled by user.\n\n";
        }
    }

    return 0;
}
