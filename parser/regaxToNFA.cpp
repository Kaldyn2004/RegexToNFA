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

    bool escape = false;

    std::queue<std::string> result;
    std::stack<State> parenStack;
    int alt = 0;
    int atom = 0;
    char prev = 0;

    for (char c: re) {
        std::cout << c;
        if (escape) {
            escape = false; // Сбрасываем флаг

            if (atom > 1) {
                result.emplace("conc");
                --atom;
            }
            result.emplace(1, '\\');
            result.emplace(1, c);

            ++atom;
            prev = 'c';
            continue;
        }

        switch (c) {
            case '\\':
                escape = true; // Устанавливаем флаг экранирования
                break;

            case '(':
                if (atom > 1) {
                    result.push("conc");
                    --atom;
                }
                parenStack.push({alt, atom});
                alt = 0;
                atom = 0;
                prev = '(';
                break;

            case '|':
                if (atom == 0) {
                    throw std::invalid_argument("Invalid regex: '|' without preceding operand.");
                }
                while (--atom > 0) {
                    result.push("conc");
                }
                ++alt;
                prev = '|';
                break;

            case ')':
                if (prev == '(')
                {
                    State top = parenStack.top();
                    parenStack.pop();
                    alt = top.alt;
                    atom = top.atom;
                    result.emplace("\u03B5");
                    ++atom;
                    break;
                }
                else
                {
                    if (parenStack.empty() || atom == 0) {
                        throw std::invalid_argument("Invalid regex: mismatched parentheses2.");
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
                }

                prev = ')';
                break;

            case '*':
            case '+':
                if (atom == 0) {
                    throw std::invalid_argument("Invalid regex: operator '" + std::string(1, c) + "' without operand.");
                }
                result.emplace(1, c);
                prev = c;
                break;

            default:
                if (atom > 1) {
                    result.emplace("conc");
                    --atom;
                }
                if (c == '?')
                {
                    result.emplace("\u03B5");
                }
                else
                {
                    result.emplace(1, c);
                }

                ++atom;
                prev = c;
                break;
        }
    }

    if (!parenStack.empty()) {
        throw std::invalid_argument("Invalid regex: mismatched parentheses1.");
    }

    while (--atom > 0) {
        result.emplace("conc");
    }
    for (; alt > 0; --alt) {
        result.emplace("|");
    }

    return result;
}

