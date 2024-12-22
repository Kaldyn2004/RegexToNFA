#include <iostream>
#include <vector>
#include <memory>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <stack>
#include <functional>
#include <set>
#include <fstream>
#include "./parser/regaxToNFA.cpp"
#include "./parser/test.cpp"

enum {
    Match = 256,
    Split = 257
};

struct State {
    int c;                     // Символ или тип состояния
    State *out = nullptr;      // Первый выход
    State *out1 = nullptr;     // Второй выход (используется для Split)
    int lastlist = 0;

    int id = 0;// Для обхода состояний
};

State matchstate = {Match};
int nstate = 0;

struct Frag {
    State *start = nullptr;
    std::vector<State **> out;

    Frag(State *start, std::vector<State **> out) : start(start), out(std::move(out)) {}
};

// Функции для работы с состояниями
State *createState(int c, State *out = nullptr, State *out1 = nullptr) {
    nstate++;
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
    if (postfix.empty()) {
        return nullptr;
    }

    std::vector<Frag> stack;
    while (!postfix.empty())
    {
        auto c = postfix.front(); // Получаем элемент из начала очереди
        postfix.pop();
        std::cout << c << std::endl;

        if (c == "conc")
        {
            Frag e2 = stack.back();
            stack.pop_back();
            Frag e1 = stack.back();
            stack.pop_back();
            patch(e1.out, e2.start);
            stack.emplace_back(e1.start, e2.out);
        }
        else if (c == "|")
        {
            Frag e2 = stack.back();
            stack.pop_back();
            Frag e1 = stack.back();
            stack.pop_back();
            State *s = createState(Split, e1.start, e2.start);
            stack.emplace_back(s, append(e1.out, e2.out));
        }
        else if (c == "?")
        {
            Frag e = stack.back();
            stack.pop_back();
            State *s = createState(Split, e.start, nullptr);
            stack.emplace_back(s, append(e.out, list1(&s->out1)));
        }
        else if (c == "*")
        {
            Frag e = stack.back();
            stack.pop_back();
            State *s = createState(Split, e.start, nullptr);
            patch(e.out, s);
            stack.emplace_back(s, list1(&s->out1));
        }
        else if (c == "+")
        {
            Frag e = stack.back();
            stack.pop_back();
            State *s = createState(Split, e.start, nullptr);
            patch(e.out, s);
            stack.emplace_back(e.start, list1(&s->out1));
        }
        else
        {
            unsigned char d = c[0];
            State *s = createState(d);
            stack.emplace_back(s, list1(&s->out));
        }
    }

    Frag e = stack.back();
    stack.pop_back();
    if (!stack.empty()) {
        std::cout << "failed" << std::endl;
        return nullptr;
    }

    patch(e.out, &matchstate);
    return e.start;
}

// Работа со списками состояний
struct List {
    std::vector<State *> states;
    int id = 0;

    void addState(State *s);
};

int listid = 0;

void List::addState(State *s) {
    if (!s || s->lastlist == id) return;
    s->lastlist = id;
    if (s->c == Split) {
        addState(s->out);
        addState(s->out1);
    } else {
        states.push_back(s);
    }
}

List *startList(State *start, List *l) {
    l->states.clear();
    l->id = ++listid;
    l->addState(start);
    return l;
}

bool isMatch(const List &l) {
    for (State *s: l.states) {
        if (s == &matchstate) {
            return true;
        }
    }
    return false;
}

void step(const List &clist, int c, List *nlist) {
    nlist->states.clear();
    nlist->id = ++listid;

    for (State *s: clist.states) {
        if (s->c == c) {
            nlist->addState(s->out);
        }
    }
}

bool match(State *start, const std::string &s) {
    List l1, l2;
    List *clist = startList(start, &l1);
    List *nlist = &l2;

    for (char c: s) {
        step(*clist, c, nlist);
        std::swap(clist, nlist);
    }

    return isMatch(*clist);
}


// Функция для записи NFA в файл
void writeNFAtoFile(State *start, const std::string &filename) {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        throw std::runtime_error("Не удалось открыть файл для записи.");
    }

    outFile << "Входной автомат (НКА с ε-переходами):\n\n";

    // Состояния
    std::set<State*> states;
    std::vector<State*> stateList;

    // Собираем все состояния
    std::function<void(State*)> collectStates = [&](State* s) {
        if (s && states.insert(s).second) { // Если состояние еще не добавлено
            stateList.push_back(s);
            if (s->c == Split) {
                collectStates(s->out);
                collectStates(s->out1);
            } else {
                collectStates(s->out);
            }
        }
    };

    for (size_t i = 0; i < stateList.size(); ++i) {
        stateList[i]->id = static_cast<int>(i); // Присваиваем ID от 0 до n-1
    }

    collectStates(start);

    // Записываем состояния
    for (State* state : stateList) {
        outFile << "q" << state->id << ";";
    }
    outFile << "\n";

    // Переходы
    for (State* state : stateList) {
        for (char c = 'a'; c <= 'z'; ++c) { // Предполагаем, что переходы только для символов a-z
            State* nextState = state->out; // Здесь нужно реализовать логику переходов
            if (nextState) {
                outFile << c << ";";
                outFile << "q" << state->id << ",q" << nextState->id << ";";
                outFile << "\n";
            }
        }
    }

    outFile.close();
}


int main(int argc, char *argv[])
{
    State *start;

    if (argc != 3) {
        std::cout << "Incorect parameters usage: programm.exe regax output.txt" << std::endl;
        return 1;
    }

    std::string inputRegax = argv[1];
    auto post = re2post(inputRegax);
    auto post1 =  reposts(inputRegax);
    std::cout << post1;

    start = post2nfa(post);

    if (!start) {
        std::cerr << "sss NFA\n";
        return 1;
    }

    try {
        writeNFAtoFile(start, argv[2]);
        std::cout << "sssss: " << argv[2] << std::endl;
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}