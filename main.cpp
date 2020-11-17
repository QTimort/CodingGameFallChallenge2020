#include <iostream>
#include <string>
#include <vector>
#include <set>
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
    set<int> exhaustedActions = set<int>();

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
    unsigned char steps = 0;
    bool rest = false;
    vector<Node *> children = vector<Node *>();
    Node *parent = nullptr;
};

static unordered_set<Node *> nodesPerDepth[TREE_DEPTH];

struct Tree
{
    Node *root = new Node();

    static inline Node *CreateNode(InvUnion invUnion, unsigned char actionIdx)
    {
        Node *parent_node = new Node;
        parent_node->invUnion = invUnion;
        parent_node->actionIdx = actionIdx;
        parent_node->steps = 0;
        return parent_node;
    }
    static inline const Node* stonkBFS(unordered_set<Node *> (&nodes)[], const int depth , InvUnion invGoal, int &nodeVisited) {
        for (int i = 0; i < depth; ++i) {
            for (const auto& node: nodes[i]) {
                ++nodeVisited;
                if (
                        node->invUnion.inv.inv0 >= invGoal.inv.inv0 &&
                        node->invUnion.inv.inv1 >= invGoal.inv.inv1 &&
                        node->invUnion.inv.inv2 >= invGoal.inv.inv2 &&
                        node->invUnion.inv.inv3 >= invGoal.inv.inv3
                ) {
                    return node;
                }
            }
        }
        return nullptr;
    }
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

inline void computeStepTree(Node *node, const Action (&actions)[]) {
    Node *nodeIt = node;
    set<unsigned char> exhaustedIdx = set<unsigned char>();
    deque<Node *> nodes = deque<Node *>();

    while (nodeIt->parent != nullptr) {
        nodes.push_front(nodeIt);
        if (nodeIt->rest) {
            break;
        }
        nodeIt = nodeIt->parent;
    }
    for (int i = 0; i < nodes.size(); ++i) {
        nodeIt = nodes[i];
        unsigned char actionIdx = nodeIt->actionIdx;
        if (exhaustedIdx.find(actionIdx) == exhaustedIdx.end()) {
            exhaustedIdx.insert(actionIdx);
            if (!nodeIt->rest)
                nodeIt->steps = (nodeIt->parent == nullptr) ? (1) : (nodeIt->parent->steps + 1);
        } else if (!actions[actionIdx].repeatable || nodes[i - 1]->actionIdx != actionIdx) {
            exhaustedIdx.clear();
            nodeIt->steps = nodes[i - 1]->steps + 2;
            nodeIt->rest = true;
        } else {
            nodeIt->steps = nodes[i - 1]->steps;
        }
    }
}

inline void buildTree(Node *node, const vector<Action> &actions, int depth, const int maxDepth) {
    InvUnion inv = node->invUnion;
    ++depth;
    for (int i = 0; i < actions.size(); i++) {
        if (depth <= maxDepth && actions[i].actionType == Cast && CAN_DO_ACTION(inv, actions[i])) {
            APPLY_ACTION(inv, actions[i])
            Node *childNode = Tree::InsertNode(node, inv, i);
            if (depth > 0) {
                nodesPerDepth[depth - 1].insert(childNode);
            }
            buildTree(childNode, actions, depth, maxDepth);
            //computeStepTree(childNode, actions);
            inv.packedInv = node->invUnion.packedInv;
        }
    }
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
            a.insert(a.begin(), it);
        }
    }
    return a;
}

inline vector<Action> getAllLearn(const Action (&actions)[], const int actionCount) {
    vector<Action> a = vector<Action>();
    for (int i = 0; i < actionCount; i++) {
        const Action &it = actions[i];
        if (it.actionType == Learn) {
            a.insert(a.begin(), it);
        }
    }
    return a;
}

inline void firstRoundCompute(vector<Action> &allCast, Tree &castTree, const InvUnion &invToSearch) {
    auto t1 = std::chrono::high_resolution_clock::now();
    buildTree(castTree.root, allCast, 0, TREE_DEPTH);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    cerr << "tree build duration: " << duration << "ms" << endl;
    t1 = std::chrono::high_resolution_clock::now();
    int count = 0;
    const Node *bfs = Tree::stonkBFS(reinterpret_cast<unordered_set<Node *> (&)[]>(nodesPerDepth), TREE_DEPTH, invToSearch, count);
    t2 = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    cerr << "tree search duration: " << duration << "ms to visit " << to_string(count) << " nodes" << endl;
    cerr << "found solution? " << ((bfs == nullptr) ? ("No") : ("Yes")) << endl;
}

inline deque<int> convertTreeSteps(const deque<int> &steps, const vector<Action> &initialCastsAndLearn) {
    deque<int> convertedSteps = deque<int>();
    unordered_set<int> exhaustedId = unordered_set<int>();
    for (int i = 0; i < steps.size(); ++i) {
        const int step = steps[i];
        cerr << "i=" << to_string(i) << " step=" << to_string(step) << endl;
        const Action &a = initialCastsAndLearn[step];
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

inline deque<int> stepsFromNode(const Node *node) {
    deque<int> steps;
    const Node *it = node;
    while (it->parent != nullptr) {
        steps.push_front(it->actionIdx);
        it = it->parent;
        cerr << to_string(it->invUnion.inv.inv0) << to_string(it->invUnion.inv.inv1) << to_string(it->invUnion.inv.inv2) << to_string(it->invUnion.inv.inv3) << endl;
    }
    cerr << endl;
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

inline void codingGameAI() {
    PlayerInfo me;
    PlayerInfo enemy;
    deque<int> steps = deque<int>();
    int targetId;
    int actionCount; // the number of spells and recipes in play
    int round = 0;
    int targetIdx = 0;
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
        invToSearch.inv.inv0 = 2;
        invToSearch.inv.inv1 = 2;
        invToSearch.inv.inv2 = 2;
        invToSearch.inv.inv3 = 0;
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
        if (round == spellsToLearn.size()) { // todo change this is just to test
            int count;
            //const Node *initialNode = Tree::stonkBFS(reinterpret_cast<unordered_set<Node *> (&)[]>(nodesPerDepth), TREE_DEPTH, invToSearch, count);
            //if (bfs == nullptr) {
            //    cerr << "Kaboom, no solution found in tree" << endl;
            //    break;
            //}
            // todo fix we cant start from the initial node
            const Node *nodeForBrew = Tree::stonkBFS(reinterpret_cast<unordered_set<Node *> (&)[]>(nodesPerDepth), TREE_DEPTH, invToSearch, count);
            if (nodeForBrew != nullptr) {
                steps = convertTreeSteps(stepsFromNode(nodeForBrew), initialCastsAndLearn);
            } else {
                cerr << "Kaboom, no solution found in tree" << endl;
            }
        }
        for (auto& i: steps)
            cerr << i << ' ';
        cerr << endl;
        if (steps.empty()) {
            if (me.exhaustedActions.empty()) {
                cout << "WAIT" << endl;
            } else {
                cout << "REST" << endl;
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
    p.exhaustedActions = set<int>();
    p.inv0 = 6;
    p.inv1 = 0;
    p.inv2 = 0;
    p.inv3 = 0;
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
    invToSearch.inv.inv0 = 0;
    invToSearch.inv.inv1 = 0;
    invToSearch.inv.inv2 = 3;
    invToSearch.inv.inv3 = 0;
    int count = 0;
    auto t1 = std::chrono::high_resolution_clock::now();
    buildTree(t.root, actions, 0, TREE_DEPTH);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    const Node *bfs = Tree::stonkBFS(reinterpret_cast<unordered_set<Node *> (&)[]>(nodesPerDepth), TREE_DEPTH, invToSearch, count);
    std::cout << duration << endl;
    if (bfs != nullptr) {
        cout<<"found solution after looking " << count << " node" << endl;
    } else
        cout<<"no solution after looking " << count << " node" << endl;
    cout << "end" << endl;
    //const vector<int> &vector1  = search(p, reinterpret_cast<Action (&)[]>(actions), -1, 9, 0, 12);
    //const vector<int> &vector2  = searchEarly(p, reinterpret_cast<Action (&)[]>(actions), -1, 9, 0, 8);
    //cout<<"nb steps "<< to_string(vector1.size()) <<endl;
    //cout<<"nb steps early "<< to_string(vector2.size()) <<endl;
}

int main()
{
    codingGameAI();
    //test();
}
