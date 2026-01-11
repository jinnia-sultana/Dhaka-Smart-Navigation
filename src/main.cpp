#include <iostream>
#include <string>
#include <iomanip>
#include <limits>
#include <vector>
#include <queue>
#include <cmath>
#include <map>
#include <algorithm>
#include <cctype>
using namespace std;

// ==================== GRAPH STRUCTURES ====================
struct Node {
    int id;
    string name;
    double x, y;
    
    Node(int i, string n, double _x, double _y) : id(i), name(n), x(_x), y(_y) {}
};

struct Edge {
    int from, to;
    double distance;
    string mode;
    
    Edge(int f, int t, double dist, string m)
        : from(f), to(t), distance(dist), mode(m) {}
};

struct RouteOption {
    string mode;
    vector<int> path;
    double distance;
    double time;
    double cost;
    double score;
    
    bool operator<(const RouteOption& other) const {
        return score < other.score;
    }
};

// ==================== GRAPH DATA ====================
vector<Node> nodes;
vector<Edge> edges;
map<int, vector<int>> adjacency;

void initializeGraph() {
    nodes = {
        Node(0, "Dhanmondi", 10, 50),
        Node(1, "Farmgate", 30, 80),
        Node(2, "Shahbag", 50, 50),
        Node(3, "Motijheel", 80, 20),
        Node(4, "Gulistan", 70, 30),
        Node(5, "Kalabagan", 20, 40),
        Node(6, "KarwanBazar", 40, 60),
        Node(7, "PressClub", 60, 40)
    };
    
    edges.clear();

    auto addBidirectional = [&](int from, int to, double dist, string mode) {
        edges.push_back(Edge(from, to, dist, mode));
        edges.push_back(Edge(to, from, dist, mode)); 
    };

    // ========== WALKING EDGES ==========
    addBidirectional(0, 1, 5.0, "walk");
    addBidirectional(1, 2, 5.0, "walk");
    addBidirectional(2, 3, 5.0, "walk");
    addBidirectional(3, 4, 5.0, "walk");
    addBidirectional(0, 5, 3.0, "walk");
    addBidirectional(5, 6, 3.0, "walk");
    addBidirectional(6, 7, 3.0, "walk");
    addBidirectional(7, 2, 3.0, "walk");
    addBidirectional(4, 6, 4.0, "walk");
    addBidirectional(4, 7, 5.0, "walk");
    addBidirectional(3, 6, 4.0, "walk");
    
    // ========== RICKSHAW EDGES ==========
    addBidirectional(0, 1, 5.0, "rickshaw");
    addBidirectional(1, 2, 5.0, "rickshaw");
    addBidirectional(2, 3, 5.0, "rickshaw");
    addBidirectional(3, 4, 5.0, "rickshaw");
    addBidirectional(0, 2, 12.0, "rickshaw"); 
    addBidirectional(1, 3, 12.0, "rickshaw");
    addBidirectional(0, 5, 3.0, "rickshaw");
    addBidirectional(5, 7, 6.0, "rickshaw");
    addBidirectional(7, 3, 8.0, "rickshaw");
    addBidirectional(4, 6, 4.0, "rickshaw");
    addBidirectional(4, 7, 5.0, "rickshaw");
    
    // ========== BUS EDGES ==========
    addBidirectional(0, 2, 12.0, "bus");
    addBidirectional(2, 4, 12.0, "bus");
    addBidirectional(0, 6, 8.0, "bus");
    addBidirectional(6, 3, 10.0, "bus");
    addBidirectional(3, 4, 5.0, "bus");
    addBidirectional(4, 6, 6.0, "bus");
    // Additional bus routes to ensure connectivity
    addBidirectional(1, 2, 5.0, "bus");
    addBidirectional(2, 7, 3.0, "bus");
    addBidirectional(1, 6, 4.0, "bus");
    
    adjacency.clear();
    for (int i = 0; i < edges.size(); i++) {
        adjacency[edges[i].from].push_back(i);
    }
}

// Heuristic function (Euclidean distance based on coordinates)
double heuristic(int n1, int n2) {
    double dx = nodes[n1].x - nodes[n2].x;
    double dy = nodes[n1].y - nodes[n2].y;
    return sqrt(dx*dx + dy*dy);
}

// ==================== SIMPLE A* (Finds shortest path) ====================
vector<int> findShortestPath(int start, int goal, string mode) {
    priority_queue<pair<double, int>, vector<pair<double, int>>, greater<pair<double, int>>> openSet;
    
    map<int, double> gScore;
    map<int, double> fScore;
    map<int, int> cameFrom;
    map<int, bool> visited;
    
    gScore[start] = 0;
    fScore[start] = heuristic(start, goal);
    openSet.push({fScore[start], start});
    
    while (!openSet.empty()) {
        int current = openSet.top().second;
        openSet.pop();
        
        if (current == goal) {
            vector<int> path;
            while (current != start) {
                path.push_back(current);
                current = cameFrom[current];
            }
            path.push_back(start);
            reverse(path.begin(), path.end());
            return path;
        }
        
        if (visited[current]) continue;
        visited[current] = true;
        
        for (int edgeIdx : adjacency[current]) {
            Edge& edge = edges[edgeIdx];
            if (edge.mode != mode) continue;
            
            int neighbor = edge.to;
            double tentative_g = gScore[current] + edge.distance;
            
            if (!gScore.count(neighbor) || tentative_g < gScore[neighbor]) {
                cameFrom[neighbor] = current;
                gScore[neighbor] = tentative_g;
                fScore[neighbor] = tentative_g + heuristic(neighbor, goal);
                openSet.push({fScore[neighbor], neighbor});
            }
        }
    }
    
    return {}; // No path found
}

// ==================== COST CALCULATIONS ====================
double calculateCost(string mode, double distance, int segments) {
    if (mode == "walk") return 0.0;
    else if (mode == "rickshaw") return distance * 30;
    else if (mode == "bus") return segments * 5;
    return 0.0;
}

double calculateTime(string mode, double distance) {
    double speed = 0.0;
    if (mode == "walk") speed = 5.0;
    else if (mode == "rickshaw") speed = 15.0;
    else if (mode == "bus") speed = 20.0;
    
    return (distance / speed) * 60;
}

// ==================== SCORE CALCULATION ====================
double calculateScore(double time, double cost, double distance, 
                     double time_weight, double expense_weight, double distance_weight) {
    // Normalize values to 0-1 scale
    double max_time = 240.0;
    double max_cost = 1000.0;
    double max_distance = 30.0;
    
    double normalized_time = time / max_time;
    double normalized_cost = cost / max_cost;
    double normalized_distance = distance / max_distance;
    
    // Calculate weighted score (lower = better)
    double score = (time_weight * normalized_time) + 
                   (expense_weight * normalized_cost) + 
                   (distance_weight * normalized_distance);
    
    return score;
}

// ==================== DISPLAY FUNCTIONS ====================
void displayMap() {
    cout << "\nDhaka City Map:\n";
    cout << "========================================\n";
    cout << "Dhanmondi ----- Farmgate ----- Shahbag\n";
    cout << "   |               |             |\n";
    cout << "   |               |             |\n";
    cout << "Kalabagan --- KarwanBazar --- PressClub\n";
    cout << "                       |\n";
    cout << "                       |\n";
    cout << "                   Motijheel ----- Gulistan\n";
    cout << "========================================\n";
}

void displayRouteDetails(RouteOption& route, int rank) {
    if (rank == 1) {
        cout << "\nRECOMMENDED ROUTE:\n";
        cout << "========================================\n";
    } else {
        cout << "\n" << rank << ". ALTERNATIVE:\n";
        cout << "----------------------------------------\n";
    }
    
    cout << "Mode: " << route.mode << "\n";
    cout << "Route: ";
    for (int j = 0; j < route.path.size(); j++) {
        cout << nodes[route.path[j]].name;
        if (j < route.path.size() - 1) cout << " -> ";
    }
    cout << "\n";
    cout << "Distance: " << fixed << setprecision(1) << route.distance << " km\n";
    cout << "Time: " << fixed << setprecision(0) << route.time << " minutes\n";
    cout << "Cost: " << fixed << setprecision(0) << route.cost << " TK";
    
    if (route.mode == "walk") cout << " (Free)";
    else if (route.mode == "rickshaw") cout << " (30 TK/km)";
    else if (route.mode == "bus") cout << " (5 TK per segment)";
    cout << "\n";
}

// ==================== LOCATION VALIDATION ====================
bool findLocationIndex(string input, int &index) {
    string locations[] = {"Dhanmondi", "Farmgate", "Shahbag", "Motijheel", "Gulistan", 
                         "Kalabagan", "KarwanBazar", "PressClub"};
    
    transform(input.begin(), input.end(), input.begin(), ::tolower);
    
    for (int i = 0; i < 8; i++) {
        string loc_lower = locations[i];
        transform(loc_lower.begin(), loc_lower.end(), loc_lower.begin(), ::tolower);
        
        if (input.find(loc_lower) != string::npos) {
            index = i;
            return true;
        }
    }
    
    return false;
}

// ==================== ROUTE CALCULATION ====================
vector<RouteOption> calculateRoutes(string from, string to, 
                                   int time_pref, int expense_pref, int distance_pref) {
    int start_idx = -1, goal_idx = -1;
    
    // Validate locations
    if (!findLocationIndex(from, start_idx)) {
        cout << "\n[ERROR] Starting location '" << from << "' not found.\n";
        return {};
    }
    
    if (!findLocationIndex(to, goal_idx)) {
        cout << "\n[ERROR] Destination '" << to << "' not found.\n";
        return {};
    }
    
    // Normalize weights
    double total_weight = time_pref + expense_pref + distance_pref;
    if (total_weight < 0.001) total_weight = 300.0; // Avoid division by zero
    
    double time_weight = time_pref / total_weight;
    double expense_weight = expense_pref / total_weight;
    double distance_weight = distance_pref / total_weight;
    
    vector<RouteOption> routes;
    vector<string> modes = {"walk", "rickshaw", "bus"};
    
    for (string mode : modes) {
        vector<int> path = findShortestPath(start_idx, goal_idx, mode);
        
        if (path.empty()) continue;
        
        double total_distance = 0;
        int segments = path.size() - 1;
        
        for (int i = 0; i < segments; i++) {
            for (Edge& edge : edges) {
                if (edge.from == path[i] && edge.to == path[i+1] && edge.mode == mode) {
                    total_distance += edge.distance;
                    break;
                }
            }
        }
        
        double time = calculateTime(mode, total_distance);
        double cost = calculateCost(mode, total_distance, segments);
        double score = calculateScore(time, cost, total_distance, 
                                     time_weight, expense_weight, distance_weight);
        
        RouteOption route;
        route.mode = mode;
        route.path = path;
        route.distance = total_distance;
        route.time = time;
        route.cost = cost;
        route.score = score;
        
        routes.push_back(route);
    }
    
    sort(routes.begin(), routes.end());
    return routes;
}

// ==================== MAIN PROGRAM ====================
int main() {
    initializeGraph();
    
    int time_val = 0, expense_val = 0, distance_val = 0;
    string from_location, to_location;
    
    system("cls");
    
    cout << "+---------------------------------------------------+\n";
    cout << "|            SMART NAVIGATION SYSTEM               |\n";
    cout << "+---------------------------------------------------+\n";
    
    displayMap();
    
    cout << "\nAvailable locations: Dhanmondi, Farmgate, Shahbag, Motijheel, Gulistan,\n";
    cout << "                     Kalabagan, KarwanBazar, PressClub\n\n";
    
    cout << "Enter starting location: ";
    getline(cin, from_location);
    cout << "Enter destination: ";
    getline(cin, to_location);
    
    cout << "\n+---------------------------------------------------+\n";
    cout << "|         SET IMPORTANCE LEVELS (0-100)           |\n";
    cout << "+---------------------------------------------------+\n";
    cout << "\nHow important is each factor to you?\n";
    cout << "Enter 0-100 for each:\n\n";
    
    cout << "Time importance: ";
    cin >> time_val;
    cout << "Expense importance: ";
    cin >> expense_val;
    cout << "Distance importance: ";
    cin >> distance_val;
    
    // Validate individual ranges
    if (time_val < 0 || time_val > 100 || 
        expense_val < 0 || expense_val > 100 || 
        distance_val < 0 || distance_val > 100) {
        cout << "\n[ERROR] Values must be between 0-100!\n";
        cout << "Press Enter to exit...";
        cin.ignore();
        cin.get();
        return 1;
    }
    
    // Calculate relative percentages
    double total = time_val + expense_val + distance_val;
    if (total < 0.001) total = 300.0;
    
    double time_percent = (time_val / total) * 100;
    double expense_percent = (expense_val / total) * 100;
    double distance_percent = (distance_val / total) * 100;
    
    cout << "\nYour preferences:\n";
    cout << "Time: " << fixed << setprecision(1) << time_percent << "%\n";
    cout << "Expense: " << fixed << setprecision(1) << expense_percent << "%\n";
    cout << "Distance: " << fixed << setprecision(1) << distance_percent << "%\n";
    
    cout << "\n    +-------------------+\n";
    cout << "    |     [   OK   ]    |\n";
    cout << "    +-------------------+\n";
    cout << "\nPress Enter to see routes...";
    cin.ignore();
    cin.get();
    
    // Calculate and display routes
    system("cls");
    vector<RouteOption> routes = calculateRoutes(from_location, to_location, 
                                                time_val, expense_val, distance_val);
    
    if (routes.empty()) {
        cout << "\nNo routes available for the specified locations.\n";
        cout << "\nPress Enter to exit...";
        cin.get();
        return 0;
    }
    
    cout << "+---------------------------------------------------+\n";
    cout << "|               RECOMMENDED ROUTES                 |\n";
    cout << "+---------------------------------------------------+\n";
    
    cout << "\nJourney: " << from_location << " -> " << to_location << "\n";
    cout << "\nYour Preferences:\n";
    cout << "Time: " << fixed << setprecision(1) << time_percent << "%\n";
    cout << "Expense: " << fixed << setprecision(1) << expense_percent << "%\n";
    cout << "Distance: " << fixed << setprecision(1) << distance_percent << "%\n";
    
    cout << "\n" << string(55, '=') << "\n";
    
    // Display ALL routes (should be 3 if all modes have paths)
    for (int i = 0; i < routes.size(); i++) {
        displayRouteDetails(routes[i], i+1);
        if (i < routes.size() - 1) {
            cout << "\n" << string(40, '-') << "\n";
        }
    }
    
    cout << "\n" << string(55, '=') << "\n";
    cout << "\nPress Enter to exit...";
    cin.get();
    
    return 0;
}
