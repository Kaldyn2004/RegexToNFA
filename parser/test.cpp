#include <iostream>
#include <vector>
#include <memory>
#include <stack>
#include <queue>
#include <set>
#include <string>

std::string reposts(const std::string &re) {
    struct State {
        int nalt = 0;
        int natom = 0;
    };

    if (re.size() >= 4000) {
        throw std::invalid_argument("Input regex is too long.");
    }

    std::string result;
    std::stack<State> parenStack;
    int nalt = 0;
    int natom = 0;

    for (char c: re) {
        switch (c) {
            case '(':
                if (natom > 1) {
                    result += '.';
                    --natom;
                }
                parenStack.push({nalt, natom});
                nalt = 0;
                natom = 0;
                break;

            case '|':
                if (natom == 0) {
                    throw std::invalid_argument("Invalid regex: '|' without preceding operand.");
                }
                while (--natom > 0) {
                    result += '.';
                }
                ++nalt;
                break;

            case ')':
                if (parenStack.empty() || natom == 0) {
                    throw std::invalid_argument("Invalid regex: mismatched parentheses.");
                }
                while (--natom > 0) {
                    result += '.';
                }
                for (; nalt > 0; --nalt) {
                    result += '|';
                }
                {
                    State top = parenStack.top();
                    parenStack.pop();
                    nalt = top.nalt;
                    natom = top.natom;
                }
                ++natom;
                break;

            case '*':
            case '+':
            case '?':
                if (natom == 0) {
                    throw std::invalid_argument("Invalid regex: operator '" + std::string(1, c) + "' without operand.");
                }
                result += c;
                break;

            default:
                if (natom > 1) {
                    result += '.';
                    --natom;
                }
                result += c;
                ++natom;
                break;
        }
    }

    if (!parenStack.empty()) {
        throw std::invalid_argument("Invalid regex: mismatched parentheses.");
    }

    while (--natom > 0) {
        result += '.';
    }
    for (; nalt > 0; --nalt) {
        result += '|';
    }

    return result;
}