#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include "./parser/regaxToNFA.cpp"
#include <map>
#include <algorithm>
#include <set>

enum {
    Match = 256,
    Split = 257
};

struct State {
    int c;                     // Символ или тип состояния
    State *out = nullptr;      // Первый выход
    State *out1 = nullptr;     // Второй выход (используется для Split)
};

State matchstate = {Match};


// Функция для генерации уникальных имен состояни

class ParserRegex {
public:
    explicit ParserRegex(std::string regexStr)
    {
        re2post(regexStr);
        State* startS = post2nfa();
        buildNFA(startS);
    }

    void PrintToFile(std::string& out)
    {
        std::ofstream file(out);

        std::cout << "States: ";
        for (const auto &state : states)
        {
            file << (finalStates.contains(state) ? ";F" : ";");
        }
        file << std::endl;
        alphabet.insert("b");

        for (const auto &state : states)
        {
            file << ";" << state;
        }
        file << std::endl;

        for (const auto& symbol : alphabet)
        {
            std::cout << symbol;
            file << symbol;
            for (const auto &state : states)
            {
                file << ";";
                if (! transitions[state][symbol].empty())
                {
                    std::string text;
                    for (const auto &to : transitions[state][symbol])
                    {
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
    std::queue<std::string> postfix;
    int stateIdCounter = 0;
    std::vector<std::string> states;
    std::set<std::string> alphabet;
    std::map<std::string, std::map<std::string, std::vector<std::string>>> transitions;
    std::string startState;
    std::set<std::string> finalStates;


    std::string generateStateId() {
        return "q" + std::to_string(stateIdCounter++);
    }

    std::queue<std::string> re2post(const std::string &re) {
        struct State {
            int alt = 0;
            int atom = 0;
        };

        std::stack<State> parenStack;
        int alt = 0;
        int atom = 0;
        char prev = 0;

        for (char c: re) {
            switch (c) {
                case '(':
                    if (atom > 1) {
                        postfix.push("conc");
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
                        postfix.push("conc");
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
                        postfix.emplace("\u03B5");
                        ++atom;
                        break;
                    }
                    else
                    {
                        if (parenStack.empty() || atom == 0) {
                            throw std::invalid_argument("Invalid regex: mismatched parentheses2.");
                        }
                        while (--atom > 0) {
                            postfix.emplace("conc");
                        }
                        for (; alt > 0; --alt) {
                            postfix.emplace("|");
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
                case '?':
                    if (atom == 0) {
                        throw std::invalid_argument("Invalid regex: operator '" + std::string(1, c) + "' without operand.");
                    }
                    postfix.emplace(1, c);
                    prev = c;
                    break;

                default:
                    if (atom > 1) {
                        postfix.emplace("conc");
                        --atom;
                    }
                    postfix.emplace(1, c);
                    ++atom;
                    prev = c;
                    break;
            }
        }

        if (!parenStack.empty()) {
            throw std::invalid_argument("Invalid regex: mismatched parentheses1.");
        }

        while (--atom > 0) {
            postfix.emplace("conc");
        }
        for (; alt > 0; --alt) {
            postfix.emplace("|");
        }
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
        for (auto outp : l) {
            *outp = s;
        }
    }

    std::vector<State **> append(const std::vector<State **> &l1, const std::vector<State **> &l2) {
        std::vector<State **> combined = l1;
        combined.insert(combined.end(), l2.begin(), l2.end());
        return combined;
    }

    State *post2nfa() {
        if (postfix.empty()) return nullptr;

        std::vector<Frag> stack;

        while (!postfix.empty()) {
            std::string token = postfix.front();
            postfix.pop();

            if (token == "conc") {
                Frag e2 = stack.back(); stack.pop_back();
                Frag e1 = stack.back(); stack.pop_back();
                patch(e1.out, e2.start);
                stack.emplace_back(e1.start, e2.out);
            } else if (token == "|") {
                Frag e2 = stack.back(); stack.pop_back();
                Frag e1 = stack.back(); stack.pop_back();
                State *s = createState(Split, e1.start, e2.start);
                stack.emplace_back(s, append(e1.out, e2.out));
            } else if (token == "*") {
                Frag e = stack.back(); stack.pop_back();
                State *s = createState(Split, e.start, nullptr);
                patch(e.out, s);
                stack.emplace_back(s, list1(&s->out1));
            } else if (token == "+") {
                Frag e = stack.back(); stack.pop_back();
                State *s = createState(Split, e.start, nullptr);
                patch(e.out, s);
                stack.emplace_back(e.start, list1(&s->out1));
            } else if (token == "?") {
                Frag e = stack.back(); stack.pop_back();
                State *s = createState(Split, e.start, nullptr);
                stack.emplace_back(s, append(e.out, list1(&s->out1)));
            } else {
                State *s = createState(token == "\u03B5" ? Split : token[0]);
                std::cout << token << std::endl;
                stack.emplace_back(s, list1(&s->out));
            }
        }

        Frag e = stack.back();
        stack.pop_back();

        patch(e.out, &matchstate);
        return e.start;
    }

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
            std::cout << current->c << std::endl;

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
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <regex> <output_file>" << std::endl;
        return 1;
    }

    std::string regex = argv[1];
    std::string outputFile = "test.txt";

    regex = "((a+b)*((c+d)*(e+f)))*";

    ParserRegex parser(regex);
    parser.PrintToFile(outputFile);
    return 0;
}
