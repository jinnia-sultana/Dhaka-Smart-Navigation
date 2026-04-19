#ifndef ROUTING_H
#define ROUTING_H

#include "graph.h"
#include <vector>
#include <queue>
#include <limits>

using namespace std;

constexpr double METRO_SPEED = 20.0;
constexpr double BUS_SPEED = 12.0;
constexpr double RICKSHAW_SPEED = 8.0;
constexpr double BIKE_SPEED = 15.0;
constexpr double WALK_SPEED = 3.0;

constexpr double METRO_COST_PER_KM = 5.0;
constexpr double BUS_COST_PER_KM = 10.0;
constexpr double RICKSHAW_COST_PER_KM = 25.0;
constexpr double BIKE_COST_PER_KM = 20.0;
constexpr double WALK_COST_PER_KM = 0.0;

struct RoutingParams {
    double w_time;
    double w_cost;
    double w_distance;
    
    void normalize() {
        double sum = w_time + w_cost + w_distance;
        if (sum > 0) {
            w_time /= sum;
            w_cost /= sum;
            w_distance /= sum;
        }
    }
};

struct RouteSegment {
    int from_node;
    int to_node;
    TransportMode mode;
    double distance;
    double time;
    double cost;
};

struct RouteResult {
    vector<int> path;
    vector<RouteSegment> segments;
    double total_time;
    double total_distance;
    double total_cost;
};

class Router {
private:
    Graph& graph;
    double heuristic(int node_id, int goal_id);
    double edgeCost(const Edge& e, const RoutingParams& params);
    double getCostPerKm(TransportMode mode);

public:
    Router(Graph& g);
    RouteResult findRoute(int start_id, int end_id, const RoutingParams& params);
};

#endif