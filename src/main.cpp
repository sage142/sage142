#include <iostream>
#include <limits>
#include <random>
#include <string>

namespace {

int readInt(const std::string& prompt, int minValue) {
    while (true) {
        std::cout << prompt;
        int value;
        if (std::cin >> value && value >= minValue) {
            return value;
        }

        std::cout << "Please enter an integer >= " << minValue << ".\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

char randomOperator(std::mt19937& rng) {
    static constexpr char ops[] = {'+', '-', '*'};
    std::uniform_int_distribution<int> opDist(0, 2);
    return ops[opDist(rng)];
}

int evaluate(int a, int b, char op) {
    switch (op) {
        case '+':
            return a + b;
        case '-':
            return a - b;
        case '*':
            return a * b;
        default:
            return 0;
    }
}

}  // namespace

int main() {
    std::random_device rd;
    std::mt19937 rng(rd());

    std::cout << "=== Math Quiz ===\n";

    const int rounds = readInt("How many questions do you want? ", 1);
    const int maxNumber = readInt("Largest number to use in problems: ", 1);

    std::uniform_int_distribution<int> numDist(0, maxNumber);

    int score = 0;

    for (int i = 1; i <= rounds; ++i) {
        const int a = numDist(rng);
        const int b = numDist(rng);
        const char op = randomOperator(rng);
        const int correct = evaluate(a, b, op);

        std::cout << "\nQ" << i << ": " << a << ' ' << op << ' ' << b << " = ";

        int answer;
        while (!(std::cin >> answer)) {
            std::cout << "Please enter a valid integer: ";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }

        if (answer == correct) {
            ++score;
            std::cout << "Correct!\n";
        } else {
            std::cout << "Not quite. Correct answer: " << correct << "\n";
        }
    }

    std::cout << "\nFinal score: " << score << "/" << rounds << "\n";

    const double percent = (static_cast<double>(score) / rounds) * 100.0;
    std::cout << "Accuracy: " << percent << "%\n";

    if (percent == 100.0) {
        std::cout << "Perfect game! Great work.\n";
    } else if (percent >= 70.0) {
        std::cout << "Nice job! Keep practicing for perfection.\n";
    } else {
        std::cout << "Good effort—try again and improve your score.\n";
    }

    return 0;
}
