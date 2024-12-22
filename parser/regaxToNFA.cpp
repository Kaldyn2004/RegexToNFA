#include <iostream>
#include <vector>
#include <memory>
#include <stack>
#include <queue>
#include <set>
#include <string>

std::queue<std::string> re2post(const std::string &re) {
    struct State {
        int alt = 0;
        int atom = 0;
    };

    std::queue<std::string> result;
    std::stack<State> parenStack;
    int alt = 0;
    int atom = 0;

    for (char c: re) {
        switch (c) {
            case '(':
                if (atom > 1) {
                    result.push("conc");
                    --atom;
                }
                parenStack.push({alt, atom});
                alt = 0;
                atom = 0;
                break;

            case '|':
                if (atom == 0) {
                    throw std::invalid_argument("Invalid regex: '|' without preceding operand.");
                }
                while (--atom > 0) {
                    result.push("conc");
                }
                ++alt;
                break;

            case ')':
                if (parenStack.empty() || atom == 0) {
                    throw std::invalid_argument("Invalid regex: mismatched parentheses.");
                }
                while (--atom > 0) {
                    result.emplace("conc");
                }
                for (; alt > 0; --alt) {
                    result.emplace("|");
                }
                {
                    State top = parenStack.top();
                    parenStack.pop();
                    alt = top.alt;
                    atom = top.atom;
                }
                ++atom;
                break;

            case '*':
            case '+':
            case '?':
                if (atom == 0) {
                    throw std::invalid_argument("Invalid regex: operator '" + std::string(1, c) + "' without operand.");
                }
                result.emplace(1, c);
                break;

            default:
                if (atom > 1) {
                    result.emplace("conc");
                    --atom;
                }
                result.emplace(1, c);
                ++atom;
                break;
        }
    }

    if (!parenStack.empty()) {
        throw std::invalid_argument("Invalid regex: mismatched parentheses.");
    }

    while (--atom > 0) {
        result.emplace("conc");
    }
    for (; alt > 0; --alt) {
        result.emplace("|");
    }

    return result;
}

