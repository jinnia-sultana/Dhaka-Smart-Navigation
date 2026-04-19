#include "routing.h"
#include <cmath>
#include <unordered_map>
#include <iostream>
#include <algorithm>

#define EARTH_RADIUS 6371000
#define M_PI 3.14159265358979323846

using namespace std;

Router::Router(Graph& g) : graph(g) {}

double Router::getCostPerKm(TransportMode mode) {
    switch(mode) {
        case TransportMode::WALK: return WALK_COST_PER_KM;
        case TransportMode::RICKSHAW: return RICKSHAW_COST_PER_KM;
        case TransportMode::BIKE: return BIKE_COST_PER_KM;
        case TransportMode::BUS: return BUS_COST_PER_KM;
        case TransportMode::METRO: return METRO_COST_PER_KM;
        default: return 0;
    }
}

double Router::heuristic(int node_id, int goal_id) {
    const Node& from = graph.nodes[node_id];
    const Node& to = graph.nodes[goal_id];
    
    double dLat = (to.lat - from.lat) * M_PI / 180.0;
    double dLon = (to.lon - from.lon) * M_PI / 180.0;
    double a = sin(dLat/2) * sin(dLat/2) +
               cos(from.lat * M_PI/180.0) * cos(to.lat * M_PI/180.0) *
               sin(dLon/2) * sin(dLon/2);
    return EARTH_RADIUS * (2 * atan2(sqrt(a), sqrt(1-a)));
}

double Router::edgeCost(const Edge& e, const RoutingParams& params) {
    double distKm = e.distance / 1000.0;
    double money = distKm * getCostPerKm(e.mode);
    
    return (e.travel_time * params.w_time) + 
           (money * params.w_cost) + 
           (e.distance * params.w_distance);
}

RouteResult Router::findRoute(int start_id, int end_id, const RoutingParams& params) {
    struct PQNode {
        int id; double f; double g;
        bool operator>(const PQNode& other) const { return f > other.f; }
    };
    
    vector<double> g_score(graph.nodes.size(), numeric_limits<double>::max());
    vector<int> came_from(graph.nodes.size(), -1);
    vector<TransportMode> came_mode(graph.nodes.size(), TransportMode::WALK);
    priority_queue<PQNode, vector<PQNode>, greater<PQNode>> pq;
    
    g_score[start_id] = 0;
    pq.push({start_id, heuristic(start_id, end_id), 0});
    
    while (!pq.empty()) {
        PQNode current = pq.top(); pq.pop();
        if (current.id == end_id) break;
        
        for (const Edge& e : graph.adj[current.id]) {
            double tentative = g_score[current.id] + edgeCost(e, params);
            if (tentative < g_score[e.to]) {
                g_score[e.to] = tentative;
                came_from[e.to] = current.id;
                came_mode[e.to] = e.mode;
                pq.push({e.to, tentative + heuristic(e.to, end_id), tentative});
            }
        }
    }
    
    RouteResult result;
    if (came_from[end_id] == -1) {
        cout << "No path found!" << endl;
        return result;
    }
    
    // Reconstruct path
    int cur = end_id;
    while (cur != -1) {
        result.path.push_back(cur);
        cur = came_from[cur];
    }
    reverse(result.path.begin(), result.path.end());
    
    // Build segments and calculate totals
    result.total_distance = 0;
    result.total_time = 0;
    result.total_cost = 0;
    
    TransportMode currentMode = came_mode[result.path[1]];
    RouteSegment currentSegment;
    currentSegment.from_node = result.path[0];
    currentSegment.mode = currentMode;
    currentSegment.distance = 0;
    currentSegment.time = 0;
    currentSegment.cost = 0;
    
    for (size_t i = 0; i < result.path.size() - 1; i++) {
        int u = result.path[i];
        int v = result.path[i+1];
        
        for (const Edge& e : graph.adj[u]) {
            if (e.to == v) {
                // Check if mode changed
                if (e.mode != currentMode) {
                    // Save previous segment
                    currentSegment.to_node = u;
                    result.segments.push_back(currentSegment);
                    
                    // Start new segment
                    currentMode = e.mode;
                    currentSegment = RouteSegment();
                    currentSegment.from_node = u;
                    currentSegment.mode = currentMode;
                    currentSegment.distance = 0;
                    currentSegment.time = 0;
                    currentSegment.cost = 0;
                }
                
                currentSegment.distance += e.distance;
                currentSegment.time += e.travel_time;
                double distKm = e.distance / 1000.0;
                currentSegment.cost += distKm * getCostPerKm(e.mode);
                
                result.total_distance += e.distance;
                result.total_time += e.travel_time;
                result.total_cost += distKm * getCostPerKm(e.mode);
                break;
            }
        }
    }
    
    // Add last segment
    currentSegment.to_node = result.path.back();
    result.segments.push_back(currentSegment);
    
    return result;
}