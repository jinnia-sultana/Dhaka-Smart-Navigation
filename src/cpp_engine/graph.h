#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <string>
#include <cstdint>

using namespace std;

enum class TransportMode { WALK, RICKSHAW, BIKE, BUS, METRO };

struct Node {
    int id;
    double lat;
    double lon;
    string name;
    
    Node() : id(-1), lat(0), lon(0), name("") {}
    Node(int i, double la, double lo, string n = "") : id(i), lat(la), lon(lo), name(n) {}
};

struct Edge {
    int to;
    double distance;
    double travel_time;
    TransportMode mode;
    
    Edge() : to(-1), distance(0), travel_time(0), mode(TransportMode::WALK) {}
    Edge(int t, double d, double tt, TransportMode m) : to(t), distance(d), travel_time(tt), mode(m) {}
};

class Graph {
public:
    vector<Node> nodes;
    vector<vector<Edge>> adj;
    
    Graph() {
        nodes.reserve(60000);
        adj.reserve(60000);
    }
    
    int addNode(double lat, double lon, const string& name = "");
    void addEdge(int from, int to, double distance, double travel_time, TransportMode mode);
    void printStats() const;
};

#endif
