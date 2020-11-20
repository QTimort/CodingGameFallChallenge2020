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

#define TREE_DEPTH (8)

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

struct Node;

struct Node {
    InvUnion invUnion;
    unsigned char actionIdx = -1;
    vector<Node *> children = vector<Node *>();
    Node *parent = nullptr;
};

struct Tree
{
    Node *root = new Node();

    static inline Node *CreateNode(InvUnion invUnion, unsigned char actionIdx)
    {
        Node *parent_node = new Node;
        parent_node->invUnion = invUnion;
        parent_node->actionIdx = actionIdx;
        return parent_node;
    }

    /*static inline const Node* BFS(const Node * const node, InvUnion invGoal, int &nodeVisited, bool exactMatch) {
        deque<const Node *> toVisit = deque<const Node *>({node});
        const Node *curNode;
        nodeVisited = 0;
        while (!toVisit.empty()) {
            ++nodeVisited;
            curNode = toVisit.front();
            toVisit.pop_front();
            if (
                    exactMatch &&
                    curNode->invUnion.inv.inv0 == invGoal.inv.inv0 &&
                    curNode->invUnion.inv.inv1 == invGoal.inv.inv1 &&
                    curNode->invUnion.inv.inv2 == invGoal.inv.inv2 &&
                    curNode->invUnion.inv.inv3 == invGoal.inv.inv3
                    ) {
                return curNode;
            }
            if (
                    !exactMatch &&
                    curNode->invUnion.inv.inv0 >= invGoal.inv.inv0 &&
                    curNode->invUnion.inv.inv1 >= invGoal.inv.inv1 &&
                    curNode->invUnion.inv.inv2 >= invGoal.inv.inv2 &&
                    curNode->invUnion.inv.inv3 >= invGoal.inv.inv3
                    ) {
                return curNode;
            }
            for (auto &n : curNode->children) {
                toVisit.insert(toVisit.begin(), n->children.begin(), n->children.end());
            }
        }
        return nullptr;
    }*/

    static inline Node *InsertNode(Node *parent, InvUnion invUnion, unsigned char actionIdx)
    {
        Node *childNode = CreateNode(invUnion, actionIdx);
        childNode->invUnion = invUnion;
        childNode->actionIdx = actionIdx;
        parent->children.push_back(childNode);
        childNode->parent = parent;
        return childNode;
    }
};

/*inline void buildTree(Node *node, const vector<Action> &actions, int depth, const int maxDepth) {
    InvUnion inv = node->invUnion;
    ++depth;
    for (int i = 0; i < actions.size(); i++) {
        if (depth <= maxDepth && actions[i].actionType == Cast && CAN_DO_ACTION(inv, actions[i])) {
            APPLY_ACTION(inv, actions[i])
            Node *childNode = Tree::InsertNode(node, inv, i);
            buildTree(childNode, actions, depth, maxDepth);
            inv.packedInv = node->invUnion.packedInv;
        }
    }
}*/

static const vector<int> EMPTY_VECTOR = vector<int>();

inline vector<int> findSolution(const InvUnion inv, const vector<Action> &casts, const vector<Action> &brews, const int maxDepth, int depth) {
    ++depth;
    InvUnion newInv = inv;
    vector<int> steps = vector<int>();
    if (depth == 0) {
        for (int j = 0; j < brews.size(); j++) {
            if (newInv.inv.inv0 >= -brews[j].delta0 && newInv.inv.inv1 >= -brews[j].delta1 && newInv.inv.inv2 >= -brews[j].delta2 && newInv.inv.inv3 >= -brews[j].delta3) {
                return vector<int>({-(j + 1)}); // done
            }
        }
    }
    for (int i = 0; i < casts.size(); i++) {
        if (depth <= maxDepth && CAN_DO_ACTION(newInv, casts[i])) {
            APPLY_ACTION(newInv, casts[i])
            for (int j = 0; j < brews.size(); j++) {
                if (newInv.inv.inv0 >= -brews[j].delta0 && newInv.inv.inv1 >= -brews[j].delta1 && newInv.inv.inv2 >= -brews[j].delta2 && newInv.inv.inv3 >= -brews[j].delta3) {
                    return vector<int>({-(j + 1), i}); // done
                }
            }
            vector<int> res = findSolution(newInv, casts, brews, maxDepth, depth);
            if (!res.empty()) {
                res.push_back(i);
                return res;
            }
            newInv.packedInv = inv.packedInv;
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


inline vector<Action> getAllLearn(const Action (&actions)[], const int actionCount) {
    vector<Action> a = vector<Action>();
    for (int i = 0; i < actionCount; i++) {
        const Action &it = actions[i];
        if (it.actionType == Learn) {
            a.insert(a.begin(), it); // order matter when buying the books
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
            cerr << it << endl;
        }
    }
    return a;
}

inline int findBrewIdxWithLowestDelta(const Action (&actions)[], const int actionCount, InvUnion inv) {
    int lowestDelta = 10;
    int lowestDeltaIdx = -1;
    for (int i = 0; i < actionCount; i++) {
        const Action &it = actions[i];
        if (it.actionType == Brew) {
            int delta = inv.inv.inv0 + it.delta0 + inv.inv.inv1 + it.delta1 + inv.inv.inv2 + it.delta2 + inv.inv.inv3 + it.delta3;
            if (delta < lowestDelta) {
                lowestDelta = delta;
                lowestDeltaIdx = i;
            }
        }
    }
    return lowestDeltaIdx;
}

inline int findBrewIdxWithHighestPrice(const Action (&actions)[], const int actionCount, InvUnion inv) {
    int highestPrice = 0;
    int highestPriceIdx = -1;
    for (int i = 0; i < actionCount; i++) {
        const Action &it = actions[i];
        if (it.actionType == Brew) {
            if (highestPrice < (it.price + it.taxCount)) {
                highestPrice = it.price + it.taxCount;
                highestPriceIdx = i;
            }
        }
    }
    return highestPriceIdx;
}

inline void firstRoundCompute(vector<Action> &allCast, Tree &castTree, const InvUnion &invToSearch) {
    auto t1 = std::chrono::high_resolution_clock::now();
    //buildTree(castTree.root, allCast, 0, TREE_DEPTH);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    cerr << "tree build duration: " << duration << "ms" << endl;
    //t1 = std::chrono::high_resolution_clock::now();
    //int count = 0;
    //const Node *bfs = Tree::stonkBFS(reinterpret_cast<unordered_set<Node *> (&)[]>(nodesPerDepth), TREE_DEPTH, invToSearch, count);
    //t2 = std::chrono::high_resolution_clock::now();
    //duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    //cerr << "tree search duration: " << duration << "ms to visit " << to_string(count) << " nodes" << endl;
    //cerr << "found solution? " << ((bfs == nullptr) ? ("No") : ("Yes")) << endl;
}

inline deque<int> convertTreeSteps(const deque<int> &steps, const vector<Action> &initialCastsAndLearn) {
    deque<int> convertedSteps = deque<int>();
    unordered_set<int> exhaustedId = unordered_set<int>();
    for (int i = 0; i < steps.size(); ++i) {
        const int step = steps[i];
        const Action &a = initialCastsAndLearn[step];
        cerr << "i=" << to_string(i) << " step=" << to_string(step) << " " << a << endl;
        if (exhaustedId.find(a.actionId) == exhaustedId.end()) {
            exhaustedId.insert(a.actionId);
            convertedSteps.push_back(a.actionId);
        } else if (!initialCastsAndLearn[steps[i - 1]].repeatable || initialCastsAndLearn[steps[i - 1]].actionId != a.actionId) {
            exhaustedId.clear();
            convertedSteps.push_back(-1); // rest
            convertedSteps.push_back(a.actionId);
            exhaustedId.insert(a.actionId);
        } else {
            convertedSteps.push_back(a.actionId);
        }
    }
    return convertedSteps;
}

inline deque<int> stepsFromNode(const Node *start, const Node *end) {
    deque<int> steps;
    const Node *it = end;
    if (start == end) {
        cerr << "start = end " << to_string(it->actionIdx) << " " << to_string(it->invUnion.inv.inv0) << to_string(it->invUnion.inv.inv1) << to_string(it->invUnion.inv.inv2) << to_string(it->invUnion.inv.inv3) << endl;
        steps.push_front(start->actionIdx);
    }
    while (it->parent != nullptr && it != start) {
        steps.push_front(it->actionIdx);
        it = it->parent;
    }
    cerr << "step from node size " << to_string(steps.size()) << endl;
    return steps;
}

inline void updateLearnedCast(
        const vector<Action> &spellsToLearn, const Action (&actions)[], const int actionCount, vector<Action> &initialCastsAndLearn
) {
    for (auto &learn : spellsToLearn) {
        int realActionIdx = -1;
        for (int i = 0; i < actionCount; ++i) {
            const Action &a = actions[i];
            if (a.actionType == Cast) {
                if (a.delta0 == learn.delta0 && a.delta1 == learn.delta1 && a.delta2 == learn.delta2 && a.delta3 == learn.delta3) {
                    realActionIdx = i;
                    break;
                }
            }
        }
        for (int i = 0; i < initialCastsAndLearn.size(); ++i) {
             Action &a = initialCastsAndLearn[i];
            if (a.actionType == Cast) {
                if (a.delta0 == learn.delta0 && a.delta1 == learn.delta1 && a.delta2 == learn.delta2 && a.delta3 == learn.delta3) {
                    a = actions[realActionIdx];
                    break;
                }
            }
        }
    }
}

inline bool isAnyCastExhausted(const Action (&actions)[], const int actionCount) {
    for (int i = 0; i < actionCount; ++i) {
        if (actions[i].actionType == Cast && !actions[i].castable) {
            return true;
        }
    }
    return false;
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
    invToSearch.inv.inv0 = 0;
    invToSearch.inv.inv1 = 0;
    invToSearch.inv.inv2 = 3;
    invToSearch.inv.inv3 = 0;
    Tree castTree;
    vector<Action> spellsToLearn;
    vector<Action> initialCastsAndLearn;

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
        const bool exhaustedCast = isAnyCastExhausted(reinterpret_cast<Action (&)[]>(actions), actionCount);
        if (round == 0) {
            initialCastsAndLearn = getAllCastAndLearnAsCast(reinterpret_cast<Action (&)[]>(actions), actionCount);
            firstRoundCompute(initialCastsAndLearn, castTree, invToSearch);
            spellsToLearn = getAllLearn(reinterpret_cast<Action (&)[]>(actions), actionCount);
            for (auto & i : spellsToLearn) {
                steps.push_front(i.actionId); // learn first spells
            }
        } else if (round == spellsToLearn.size()) {
            updateLearnedCast(spellsToLearn, reinterpret_cast<Action (&)[]>(actions), actionCount, initialCastsAndLearn);
        }
        if (round >= spellsToLearn.size() && ((targetIdx == -1 && steps.empty()) || actions[targetIdx].actionId != targetId)) {
            if (exhaustedCast) {
                steps.clear();
                steps.push_front(-1); // rest
            } else {
                const vector<Action> &brews = getAllBrews(reinterpret_cast<Action (&)[]>(actions), actionCount);
                const vector<int> solution = findSolution(invToSearch, initialCastsAndLearn, brews, TREE_DEPTH, 0);
                cerr << "-- solution --" << endl;
                for (auto& i: solution)
                    cerr << i << ' ';
                cerr << "-- end solu --" << endl;
                if (solution.empty()) {
                    // to do random cast
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
                    steps.clear();
                    for (int i = 1; i < solution.size(); ++ i) {
                        steps.push_front(solution[i]);
                    }
                    steps = convertTreeSteps(steps, initialCastsAndLearn);
                }
                //int count;
                // todo if we dont find the beginning node, we should try to any recipe that reduce the size of our inv
                //const Node *initialNode = Tree::BFS(castTree.root, invToSearch, count, true);
                //cerr << "nodes traversed in initial search " << to_string(count) << endl;
                /*if (initialNode == nullptr) {
                    cerr << "Kaboom, no initial solution found in tree" << endl;
                    for (int i = 0; i < actionCount; ++i) {
                        const Action &a = actions[i];
                        if (a.actionType == Cast && CAN_DO_ACTION(invToSearch, a)) {
                            steps.push_front(a.actionId);
                        }
                    }
                } else {
                    targetIdx = findBrewIdxWithLowestDelta(reinterpret_cast<Action (&)[]>(actions), actionCount,
                                                           invToSearch);
                    Action targetBrew = actions[targetIdx];
                    targetId = targetBrew.actionId;
                    invToSearch.inv.inv0 = -targetBrew.delta0;
                    invToSearch.inv.inv1 = -targetBrew.delta1;
                    invToSearch.inv.inv2 = -targetBrew.delta2;
                    invToSearch.inv.inv3 = -targetBrew.delta3;
                    cerr << "target brew is " << targetId << endl;
                    // todo check if we already have the inventory required to make the brew, and if so we can skip this
                    const Node *nodeForBrew = Tree::BFS(initialNode, invToSearch, count, false);
                    cerr << "nodes traversed in target search " << to_string(count) << endl;
                    if (nodeForBrew != nullptr) {
                        cerr << to_string(initialNode->invUnion.inv.inv0) << to_string(initialNode->invUnion.inv.inv1) << to_string(initialNode->invUnion.inv.inv2) << to_string(initialNode->invUnion.inv.inv3) << endl;
                        cerr << to_string(nodeForBrew->invUnion.inv.inv0) << to_string(nodeForBrew->invUnion.inv.inv1) << to_string(nodeForBrew->invUnion.inv.inv2) << to_string(nodeForBrew->invUnion.inv.inv3) << endl;
                        steps = convertTreeSteps(stepsFromNode(initialNode, nodeForBrew), initialCastsAndLearn);
                        cerr << "step  size " << to_string(steps.size()) << endl;
                    } else {
                        cerr << "Kaboom, no end solution found in tree" << endl;
                        invToSearch.inv.inv0 = me.inv0;
                        invToSearch.inv.inv1 = me.inv1;
                        invToSearch.inv.inv2 = me.inv2;
                        invToSearch.inv.inv3 = me.inv3;
                        for (int i = 0; i < actionCount; ++i) {
                            const Action &a = actions[i];
                            if (a.actionType == Cast && CAN_DO_ACTION(invToSearch, a)) {
                                steps.push_front(a.actionId);
                            }
                        }
                    }
                }*/
            }
        }
        for (auto& i: steps)
            cerr << i << ' ';
        cerr << endl;
        if (steps.empty()) {
            if (actions[targetIdx].actionId == targetId) {
                cout << "BREW " << to_string(targetId) << endl;
                targetId = -1;
                targetIdx = -1;
                if (exhaustedCast) {
                    steps.push_front(-1); // rest
                }
            } else {
                if (!exhaustedCast) {
                    cout << "WAIT" << endl;
                } else {
                    cout << "REST" << endl;
                }
            }
        } else {
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
            /*{90, Cast,-3, 0, 0, 1,0,-1,-1,1,1},
            {91, Cast,3, -1, 0, 0,0,-1,-1,1,1},
            {92, Cast,1, 1, 0, 0,0,-1,-1,1,1},
            {93, Cast,0, 0, 1, 0,0,-1,-1,1,1},
            {94, Cast,3, 0, 0, 0,0,-1,-1,1,1},
            {95, Cast,2, 3, -2, 0,0,-1,-1,1,1},
            {96, Cast,2, 1, -2, 1,0,-1,-1,1,1},
            {97, Cast,3, 0, 1, -1,0,-1,-1,1,1},
            {98, Cast,3, -2, 1, 0,0,-1,-1,1,1},
            {99, Cast,2, -3, 2, 0,0,-1,-1,1,1},
            {100, Cast,2, 2, 0, -1,0,-1,-1,1,1},
            {101, Cast,-4, 0, 2, 0,0,-1,-1,1,1},
            {102, Cast,2, 1, 0, 0,0,-1,-1,1,1},
            {103, Cast,4, 0, 0, 0,0,-1,-1,1,1},
            {104, Cast,0, 0, 0, 1,0,-1,-1,1,1},
            {105, Cast,0, 2, 0, 0,0,-1,-1,1,1},
            {106, Cast,1, 0, 1, 0,0,-1,-1,1,1},
            {107, Cast,-2, 0, 1, 0,0,-1,-1,1,1},
            {108, Cast,-1, -1, 0, 1,0,-1,-1,1,1},
            {109, Cast,0, 2, -1, 0,0,-1,-1,1,1},
            {110, Cast,2, -2, 0, 1,0,-1,-1,1,1},
            {111, Cast,-3, 1, 1, 0,0,-1,-1,1,1},
            {112, Cast,0, 2, -2, 1,0,-1,-1,1,1},
            {113, Cast,1, -3, 1, 1,0,-1,-1,1,1},
            {114, Cast,0, 3, 0, -1,0,-1,-1,1,1},
            {115, Cast,0, -3, 0, 2,0,-1,-1,1,1},
            {116, Cast,1, 1, 1, -1,0,-1,-1,1,1},
            {117, Cast,1, 2, -1, 0,0,-1,-1,1,1},
            {118, Cast,4, 1, -1, 0,0,-1,-1,1,1},
            {119, Cast,-5, 0, 0, 2,0,-1,-1,1,1},
            {120, Cast,-4, 0, 1, 1,0,-1,-1,1,1},
            {121, Cast,0, 3, 2, -2,0,-1,-1,1,1},
            {122, Cast,1, 1, 3, -2,0,-1,-1,1,1},
            {123, Cast,-5, 0, 3, 0,0,-1,-1,1,1},
            {124, Cast,-2, 0, -1, 2,0,-1,-1,1,1},
            {125, Cast,0, 0, -3, 3,0,-1,-1,1,1},
            {126, Cast,0, -3, 3, 0,0,-1,-1,1,1},
            {127, Cast,-3, 3, 0, 0,0,-1,-1,1,1},
            {128, Cast,-2, 2, 0, 0,0,-1,-1,1,1},
            {129, Cast,0, 0, -2, 2,0,-1,-1,1,1},
            {130, Cast,0, -2, 2, 0,0,-1,-1,1,1},
            {131, Cast,0, 0, 2, -1,0,-1,-1,1,1},*/
    });
    Tree t;
    InvUnion invToSearch;
    invToSearch.inv.inv0 = 3;
    invToSearch.inv.inv1 = 3;
    invToSearch.inv.inv2 = 0;
    invToSearch.inv.inv3 = 0;
    int count = 0;
    auto t1 = std::chrono::high_resolution_clock::now();
    //buildTree(t.root, actions, 0, TREE_DEPTH);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    //const Node *bfs = Tree::BFS(t.root, invToSearch, count, true);
    std::cout << duration << endl;
    //if (bfs != nullptr) {
        cout<<"found solution after looking " << count << " node" << endl;
    //} else
        cout<<"no solution after looking " << count << " node" << endl;

    const vector<Action> initialCastsAndLearn = getAllCastAndLearnAsCast(reinterpret_cast<Action (&)[]>(*actions.data()), actions.size());
    const vector<Action> &brews = getAllBrews(reinterpret_cast<Action (&)[]>(*actions.data()), actions.size());
    const vector<int> solution = findSolution(invToSearch, initialCastsAndLearn, brews, TREE_DEPTH, 0);
    cout << "end" << endl;
    //const vector<int> &vector1  = search(p, reinterpret_cast<Action (&)[]>(actions), -1, 9, 0, 12);
    //const vector<int> &vector2  = searchEarly(p, reinterpret_cast<Action (&)[]>(actions), -1, 9, 0, 8);
    //cout<<"nb steps "<< to_string(vector1.size()) <<endl;
    //cout<<"nb steps early "<< to_string(vector2.size()) <<endl;
}

int main()
{
    //codingGameAI();
    test();
}
