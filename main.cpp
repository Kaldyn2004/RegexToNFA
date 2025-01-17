#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include "./parser/regaxToNFA.cpp"
#include <map>
#include <algorithm>
#include <set>
#include <cstring>

enum {
    Match = 256,
    Split = 257
};

struct State {
    int c;
    State *out = nullptr;
    State *out1 = nullptr;
};

State matchstate = {Match};
int stateIdCounter = 0;

std::string generateStateId() {
    return "q" + std::to_string(stateIdCounter++);
}

struct Frag {
    State *start;
    std::vector<State **> out;

    Frag(State *start, std::vector<State **> out) : start(start), out(std::move(out)) {}
};

State *createState(int c, State *out = nullptr, State *out1 = nullptr) {
    return new State{c, out, out1};
}

std::vector<State **> list1(State **outp) {
    return {outp};
}

void patch(std::vector<State **> &l, State *s) {
    for (auto outp: l) {
        *outp = s;
    }
}

std::vector<State **> append(const std::vector<State **> &l1, const std::vector<State **> &l2) {
    std::vector<State **> combined = l1;
    combined.insert(combined.end(), l2.begin(), l2.end());
    return combined;
}

State *post2nfa(std::queue<std::string> &postfix) {
    if (postfix.empty()) return nullptr;

    std::vector<Frag> stack;

    bool escape = false;

    while (!postfix.empty()) {
        std::string token = postfix.front();
        postfix.pop();

        if (escape) {
            escape = false;
            State *s = createState(token == "\u03B5" ? Split : token[0]);
            stack.emplace_back(s, list1(&s->out));
        } else if (token == "\\") {
            escape = true;

        } else if (token == "conc") {
            Frag e2 = stack.back();
            stack.pop_back();
            Frag e1 = stack.back();
            stack.pop_back();
            patch(e1.out, e2.start);
            stack.emplace_back(e1.start, e2.out);
        } else if (token == "|") {
            Frag e2 = stack.back();
            stack.pop_back();
            Frag e1 = stack.back();
            stack.pop_back();
            State *s = createState(Split, e1.start, e2.start);
            stack.emplace_back(s, list1(&s->out1));
        } else if (token == "*") {
            Frag e = stack.back();
            stack.pop_back();
            State *s = createState(Split, e.start, nullptr);
            patch(e.out, s);
            stack.emplace_back(s, list1(&s->out1));
        } else if (token == "+") {
            Frag e = stack.back();
            stack.pop_back();
            State *s = createState(Split, e.start, nullptr);
            patch(e.out, s);
            stack.emplace_back(e.start, list1(&s->out1));
        } else if (token == "?") {
            Frag e = stack.back();
            stack.pop_back();
            State *s = createState(Split, e.start, nullptr);
            stack.emplace_back(s, append(e.out, list1(&s->out1)));
        } else {
            State *s = createState(token == "\u03B5" ? Split : token[0]);
            stack.emplace_back(s, list1(&s->out));
        }
    }

    Frag e = stack.back();
    stack.pop_back();

    patch(e.out, &matchstate);
    return e.start;
}


class Moore {
public:
    void buildNFA(State *start) {
        std::queue<State *> queue;
        std::map<State *, std::string> stateMap;

        queue.push(start);
        stateMap[start] = generateStateId();

        while (!queue.empty()) {
            State *current = queue.front();
            queue.pop();

            std::string currentId = stateMap[current];
            if (std::find(states.begin(), states.end(), currentId) == states.end()) {
                states.push_back(currentId);
            }

            if (current->c == Match) {
                finalStates.insert(currentId);
                continue;
            }

            std::string symbol = (current->c == Split) ? "\u03B5" : std::string(1, static_cast<char>(current->c));
            alphabet.insert(symbol);

            if (current->out && stateMap.find(current->out) == stateMap.end()) {
                stateMap[current->out] = generateStateId();
                queue.push(current->out);
            }

            if (current->out1 && stateMap.find(current->out1) == stateMap.end()) {
                stateMap[current->out1] = generateStateId();
                queue.push(current->out1);
            }

            if (current->out) {
                transitions[currentId][symbol].push_back(stateMap[current->out]);
            }

            if (current->out1) {
                transitions[currentId][symbol].push_back(stateMap[current->out1]);
            }
        }

        startState = stateMap[start];
    }

    void PrintNFA(std::string& string) {
        std::ofstream file(string);
        for (const auto &state: states) {
            file << (finalStates.contains(state) ? ";F" : ";");
        }
        file << std::endl;
        alphabet.insert("b");

        for (const auto &state: states) {
            file << ";" << state;
        }
        file << std::endl;

        for (const auto &symbol: alphabet) {
            file << symbol;
            for (const auto &state: states) {
                file << ";";
                if (!transitions[state][symbol].empty()) {
                    std::string text;
                    for (const auto &to: transitions[state][symbol]) {
                        text += to + ",";
                    }
                    text.pop_back();
                    file << text;
                }
            }
            file << std::endl;
        }
    }

private:
    std::vector<std::string> states;
    std::set<std::string> alphabet;
    std::map<std::string, std::map<std::string, std::vector<std::string>>> transitions;
    std::string startState;
    std::set<std::string> finalStates;

};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <output_file.csv> <regex>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string outFileName = argv[1];

    const char* original = argv[2];
    size_t length = std::strlen(original);

    std::string regex;


    for (size_t i = 0; i < length; ++i) {
        int ch = static_cast<int>(original[i]);
        std::cout << ch << std::endl;
        if (ch < 0) {
            regex += '?';
            std::cout << "Invalid Character at " << i << ": " << original[i]
                      << " (ASCII: " << static_cast<int>(ch) << ")" << std::endl;
        } else {
            regex += original[i];
        }
    }

    std::queue<std::string> postfix = re2post(regex);
    State *nfa = post2nfa(postfix);
    Moore mooreAutomat;
    mooreAutomat.buildNFA(nfa);

    mooreAutomat.PrintNFA(outFileName);

    return 0;
}
