#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <deque>
#include <unordered_set>
#include <unordered_map>

#pragma GCC optimize("Ofast")
#pragma GCC optimize("inline")
#pragma GCC optimize("omit-frame-pointer")
#pragma GCC optimize("unroll-loops")
#pragma ide diagnostic ignored "EndlessLoop"

#define CAN_DO_ACTION(INV, ACTION) \
    (((INV).inv.inv0 + (ACTION).delta0 >= 0) && \
    ((INV).inv.inv1 + (ACTION).delta1 >= 0) && \
    ((INV).inv.inv2 + (ACTION).delta2 >= 0) && \
    ((INV).inv.inv3 + (ACTION).delta3 >= 0) && \
    ((INV).inv.inv0 + (ACTION).delta0 + (INV).inv.inv1 + (ACTION).delta1 + \
    (INV).inv.inv2 + (ACTION).delta2 + (INV).inv.inv3 + (ACTION).delta3) < 11)

#define APPLY_ACTION(INV, ACTION) \
if ((ACTION).actionType == Cast) { \
    (INV).inv.inv0 += (ACTION).delta0; \
    (INV).inv.inv1 += (ACTION).delta1; \
    (INV).inv.inv2 += (ACTION).delta2; \
    (INV).inv.inv3 += (ACTION).delta3; \
}

#define REST_ACTION_ID (100)

#define SPELLS_TO_LEARN (10)
#define TREE_DEPTH (8)
#define MIN_PRICE (9)
#define MIN_INV_FOR_SEARCH (3)

using namespace std;

enum ActionType {
    Brew,
    Cast,
    OpponentCast,
    Learn,
};

inline string ActionToString(ActionType a){
    switch (a) {
        case Brew:   return "BREW";
        case Cast:   return "CAST";
        case OpponentCast: return "OPPONENT_CAST";
        case Learn: return "LEARN";
    }
    throw runtime_error("Invalid Action");
}

inline ActionType StringToAction(const string &str) {
    if (str == "BREW")
        return Brew;
    if (str == "CAST")
        return Cast;
    if (str == "OPPONENT_CAST")
        return OpponentCast;
    if (str == "LEARN")
        return Learn;
    throw runtime_error("Invalid string for Action");
}

struct PlayerInfo {
    friend ostream &operator<<(ostream &os, const PlayerInfo &info) {
        os << "inv0: " << info.inv0 << " inv1: " << info.inv1 << " inv2: " << info.inv2 << " inv3: " << info.inv3
           << " score: " << info.score;
        return os;
    }
    int inv0 = 0; // tier-0 ingredients in inventory
    int inv1 = 0;
    int inv2 = 0;
    int inv3 = 0;
    int score = 0; // amount of rupees

    inline static void readNextPlayerInfo(PlayerInfo &p) {
        cin >> p.inv0 >> p.inv1 >> p.inv2 >> p.inv3 >> p.score; cin.ignore();
    }
};

struct Action {
    friend ostream &operator<<(ostream &os, const Action &action) {
        os << "actionId: " << action.actionId << " actionType: " << action.actionType << " delta0: " << action.delta0
           << " delta1: " << action.delta1 << " delta2: " << action.delta2 << " delta3: " << action.delta3 << " price: "
           << action.price << " tomeIndex: " << action.tomeIndex << " taxCount: " << action.taxCount << " castable: "
           << action.castable << " repeatable: " << action.repeatable;
        return os;
    }

    int actionId; // the unique ID of this spell or recipe
    ActionType actionType; // in the first league: BREW; later: CAST, OPPONENT_CAST, LEARN, BREW
    int delta0; // tier-0 ingredient change
    int delta1; // tier-1 ingredient change
    int delta2; // tier-2 ingredient change
    int delta3; // tier-3 ingredient change
    int price; // the price in rupees if this is a potion
    int tomeIndex; // in the first two leagues: always 0; later: the index in the tome if this is a tome spell, equal to the read-ahead tax
    int taxCount; // in the first two leagues: always 0; later: the amount of taxed tier-0 ingredients you gain from learning this spell
    bool castable; // in the first league: always 0; later: 1 if this is a castable player spell
    bool repeatable; // for the first two leagues: always 0; later: 1 if this is a repeatable player spell

    inline static Action readNextAction() {
        Action a = {};
        string actionType;
        cin >> a.actionId >> actionType >> a.delta0 >> a.delta1 >> a.delta2 >> a.delta3 >> a.price >> a.tomeIndex >>
            a.taxCount >> a.castable >> a.repeatable; cin.ignore();
        a.actionType = StringToAction(actionType);
        return a;
    }

    inline static void readNextActions(Action (&actions)[], const int actionCount) {
        for (int i = 0; i < actionCount; i++) {
            actions[i] = readNextAction();
        }
    }
};

union InvUnion {
    unsigned int packedInv;
    struct {
        unsigned char inv0;
        unsigned char inv1;
        unsigned char inv2;
        unsigned char inv3;
    } inv;

    InvUnion() {
        this->packedInv = 0;
        this->inv.inv0 = 0;
        this->inv.inv1 = 0;
        this->inv.inv2 = 0;
        this->inv.inv3 = 0;
    }
};

static const vector<int> EMPTY_VECTOR = vector<int>();

struct SearchNode {
    InvUnion inv;
    vector<int> actions; // -100 = rest
    unsigned char depth = 0;
    SearchNode(const InvUnion &inv, const vector<int> &actions) : inv(inv), actions(actions) {}
};

inline vector<int> bfs(const InvUnion startInv, const vector<Action> &casts, const vector<Action> &brews, const int maxDepth) {
    deque<SearchNode> nodes = deque<SearchNode>();
    nodes.emplace_back(startInv, vector<int>());
    InvUnion tmpInv;
    while (!nodes.empty()) {
        const SearchNode node = nodes.front();
        unordered_set<int> exhaustedActions = unordered_set<int>();
        nodes.pop_front();
        bool restPresent = false;
        for (auto it = node.actions.rbegin(); it != node.actions.rend(); ++it) {
            int value = (*it);
            if (value == REST_ACTION_ID) {
                restPresent = true;
                break;
            }
            exhaustedActions.insert(value);
        }
        for (int cast = 0; cast < casts.size(); ++cast) {
            tmpInv = node.inv;
            if ((restPresent || casts[cast].castable) && (exhaustedActions.find(cast) == exhaustedActions.end() || (node.actions.back() == cast && casts[cast].repeatable))) {
                if (CAN_DO_ACTION(tmpInv, casts[cast])) {
                    APPLY_ACTION(tmpInv, casts[cast]);
                    for (int brew = 0; brew < brews.size(); ++brew) {
                        if ((brews[brew].price + brews[brew].tomeIndex > MIN_PRICE) && tmpInv.inv.inv0 >= -brews[brew].delta0 && tmpInv.inv.inv1 >= -brews[brew].delta1 &&
                            tmpInv.inv.inv2 >= -brews[brew].delta2 && tmpInv.inv.inv3 >= -brews[brew].delta3) {
                            vector<int> actions = node.actions;
                            actions.push_back(cast);
                            actions.push_back(-brew - 1);
                            std::reverse(actions.begin(), actions.end());
                            return actions;
                        }
                    }
                    if (node.depth < maxDepth) {
                        SearchNode searchNode = SearchNode(tmpInv, node.actions);
                        if (!node.actions.empty() && node.actions.back() == cast && casts[cast].repeatable) {
                            searchNode.depth = node.depth;
                            searchNode.actions.push_back(cast);
                            nodes.push_front(searchNode);
                        } else {
                            searchNode.depth = node.depth + 1;
                            searchNode.actions.push_back(cast);
                            nodes.push_back(searchNode);
                        }
                    }
                }
            }
            if (!exhaustedActions.empty()) {
                SearchNode searchNode = SearchNode(node.inv, node.actions);
                searchNode.depth = node.depth + 1;
                searchNode.actions.push_back(REST_ACTION_ID);
                nodes.push_back(searchNode);
            }
        }
    }
    return EMPTY_VECTOR;
}

inline vector<Action> getAllCastAndLearnAsCast(const Action (&actions)[], const int actionCount) {
    vector<Action> a = vector<Action>();
    for (int i = 0; i < actionCount; i++) {
        const Action &it = actions[i];
        if (it.actionType == Cast) {
            a.push_back(it);
        } else if (it.actionType == Learn) {
            a.push_back(it);
            a.back().actionType = Cast;
        }
    }
    return a;
}

inline vector<Action> getAllCast(const Action (&actions)[], const int actionCount) {
    vector<Action> a = vector<Action>();
    for (int i = 0; i < actionCount; i++) {
        const Action &it = actions[i];
        if (it.actionType == Cast) {
            a.push_back(it);
        }
    }
    return a;
}

inline vector<Action> getAllBrews(const Action (&actions)[], const int actionCount) {
    vector<Action> a = vector<Action>();
    for (int i = 0; i < actionCount; i++) {
        const Action &it = actions[i];
        if (it.actionType == Brew) {
            a.push_back(it);
        }
    }
    return a;
}

inline deque<int> convertTreeSteps(const deque<int> &steps, const vector<Action> &initialCastsAndLearn) {
    deque<int> convertedSteps = deque<int>();
    for (int i = 0; i < steps.size(); ++i) {
        const int step = steps[i];
        if (step == REST_ACTION_ID) {
            convertedSteps.push_back(-1); // rest
            cerr << "i=" << to_string(i) << " step=" << to_string(step) << " " << "REST" << endl;
        } else {
            const Action &a = initialCastsAndLearn[step];
            cerr << "i=" << to_string(i) << " step=" << to_string(step) << " " << a << endl;
            convertedSteps.push_back(a.actionId);
        }
    }
    return convertedSteps;
}

inline void codingGameAI() {
    PlayerInfo me;
    PlayerInfo enemy;
    deque<int> steps = deque<int>();
    int targetId = -1;
    int actionCount; // the number of spells and recipes in play
    int round = 0;
    int targetIdx = -1;
    unordered_map<unsigned char, unsigned char> learnToCastIdx;
    InvUnion invToSearch;

    while ("alive") {
        cin >> actionCount; cin.ignore();
        Action actions[actionCount];
        Action::readNextActions(reinterpret_cast<Action (&)[]>(actions), actionCount);
        PlayerInfo::readNextPlayerInfo(me);
        PlayerInfo::readNextPlayerInfo(enemy);
        invToSearch.inv.inv0 = me.inv0;
        invToSearch.inv.inv1 = me.inv1;
        invToSearch.inv.inv2 = me.inv2;
        invToSearch.inv.inv3 = me.inv3;
        if (round < SPELLS_TO_LEARN) {
            steps.clear();
            for (int i = 0; i < actionCount; ++i) {
                if (actions[i].actionType == Learn && actions[i].tomeIndex == 0) {
                    steps.push_back(actions[i].actionId);
                    break;
                }
            }
        } else if ((targetIdx == -1 && steps.empty()) || actions[targetIdx].actionId != targetId) {
            steps.clear();
            const vector<Action> &casts = getAllCast(reinterpret_cast<Action (&)[]>(actions), actionCount);
            const vector<Action> &brews = getAllBrews(reinterpret_cast<Action (&)[]>(actions), actionCount);
            if ((invToSearch.inv.inv0 + invToSearch.inv.inv1 + invToSearch.inv.inv2 + invToSearch.inv.inv3) < MIN_INV_FOR_SEARCH) {
                for (auto &a : casts) {
                    if (a.castable && a.actionType == Cast && CAN_DO_ACTION(invToSearch, a)) {
                        steps.push_front(a.actionId);
                        break;
                    }
                }
                if (steps.empty()) {
                    steps.push_front(-1);
                }
            } else {
                const vector<int> solution = bfs(invToSearch, casts, brews, TREE_DEPTH);
                cerr << "-- solution --" << endl;
                for (auto &i: solution)
                    cerr << i << ' ';
                cerr << "-- end solu --" << endl;
                if (solution.empty()) {
                    for (auto &a : casts) {
                        if (a.castable && a.actionType == Cast && CAN_DO_ACTION(invToSearch, a)) {
                            steps.push_front(a.actionId);
                            break;
                        }
                    }
                    if (steps.empty()) {
                        steps.push_front(-1);
                    }
                } else {
                    const int brewIdx = -solution.front() - 1;
                    const Action &brew = brews[brewIdx];
                    targetId = brew.actionId;
                    for (int i = 0; i < actionCount; ++i) {
                        const Action &a = actions[i];
                        if (a.actionId == targetId) {
                            targetIdx = i;
                            break;
                        }
                    }
                    cerr << "target brew is " << targetId << endl;
                    for (int i = 1; i < solution.size(); ++i) {
                        steps.push_front(solution[i]);
                    }
                    steps = convertTreeSteps(steps, casts);
                    steps.push_back(targetId);
                }
            }
        }
        for (auto& i: steps)
            cerr << i << ' ';
        cerr << endl;
        if (!steps.empty()) {
            int actionId = steps.front();
            int actionIdx = -1;
            if (actionId == -1) {
                cout << "REST" << endl;
            } else {
                int nbRepeat = 0;
                while (steps.size() > 1 && actionId == (steps[1])) {
                    ++nbRepeat;
                    steps.pop_front();
                }
                for (int i = 0; i < actionCount; ++i) {
                    if (actions[i].actionId == actionId) {
                        actionIdx = i;
                        break;
                    }
                }
                if (nbRepeat > 0) {
                    cout << ActionToString(actions[actionIdx].actionType) << " " << actions[actionIdx].actionId << " " << to_string(nbRepeat + 1) << endl;
                } else {
                    cout << ActionToString(actions[actionIdx].actionType) << " " << actions[actionIdx].actionId << endl;
                }
            }
            if (!steps.empty()) {
                steps.pop_front();
            }

        }
        round++;
    }
}

inline void test() {
    PlayerInfo p = {};
    p.inv0 = 0;
    p.inv1 = 3;
    p.inv2 = 0;
    p.inv3 = 2;
    p.score = 0;
    vector<Action> actions = vector<Action>({
            {46, Brew, -2,-3,0,0,8,0,0,1,1},
            {49, Brew, 0,-5,0,0,10,0,0,1,1},
            {54, Brew, 0,-2,0,-2,12,0,0,1,1},
            {69, Brew, -2,-2,-2,0,12,0,0,1,1},
            {65, Brew, 0,0,0,-5,20,0,0,1,1},
            {78, Cast, 2,0,0,0,0,-1,-1,1,0},
            {79, Cast, -1,1,0,0,0,-1,-1,1,0},
            {80, Cast, 0,-1,1,0,0,-1,-1,1,1},
            {81, Cast, -2,2,0,0,0,-1,-1,1,0},
    });
    InvUnion invToSearch;
    invToSearch.inv.inv0 = 0;
    invToSearch.inv.inv1 = 0;
    invToSearch.inv.inv2 = 0;
    invToSearch.inv.inv3 = 0;
    auto t1 = std::chrono::high_resolution_clock::now();
    const vector<Action> initialCastsAndLearn = getAllCastAndLearnAsCast(reinterpret_cast<Action (&)[]>(*actions.data()), actions.size());
    const vector<Action> &brews = getAllBrews(reinterpret_cast<Action (&)[]>(*actions.data()), actions.size());
    const vector<int> solution = bfs(invToSearch, initialCastsAndLearn, brews, TREE_DEPTH);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::cout << duration << endl;
    if (!solution.empty()) {
        cout<<"found solution" << endl;
    } else {
        cout << "no solution found" << endl;
    }

    cout << "end" << endl;
}

int main()
{
    codingGameAI();
    //test();
}
