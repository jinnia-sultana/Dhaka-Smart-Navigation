#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

using namespace std;

// Transport modes with bit flags for easy combination
enum class TransportMode : uint8_t {
    WALK = 1 << 0,
    RICKSHAW = 1 << 1,
    BIKE = 1 << 2,
    BUS = 1 << 3,
    METRO = 1 << 4,
    ALL = WALK | RICKSHAW | BIKE | BUS | METRO
};

// Overload bitwise operators for mode combinations
inline TransportMode operator|(TransportMode a, TransportMode b) {
    return static_cast<TransportMode>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline TransportMode operator&(TransportMode a, TransportMode b) {
    return static_cast<TransportMode>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

// Node structure with compact storage
struct Node {
    int id;
    float lat;           // Using float saves memory (4 bytes vs 8)
    float lon;           // Still precise enough for city navigation
    string name;
    uint16_t walkableNeighbors;  // Count for quick heuristics
    
    Node() : id(-1), lat(0), lon(0), walkableNeighbors(0) {}
    
    // For spatial indexing
    uint32_t getGridCell(float gridSize = 0.01) const {
        uint32_t x = static_cast<uint32_t>((lat + 90) / gridSize);
        uint32_t y = static_cast<uint32_t>((lon + 180) / gridSize);
        return (x << 16) | y;  // Pack into 32-bit
    }
};

// Compact edge structure (24 bytes)
struct Edge {
    uint32_t to;          // 4 bytes
    float distance;       // 4 bytes
    float travel_time;    // 4 bytes
    TransportMode mode;   // 1 byte
    uint8_t padding[3];   // 3 bytes padding (for alignment)
    
    Edge() : to(0), distance(0), travel_time(0), mode(TransportMode::WALK) {}
    
    Edge(uint32_t toNode, float dist, float time, TransportMode m)
        : to(toNode), distance(dist), travel_time(time), mode(m) {}
};

// Graph statistics
struct GraphStats {
    size_t nodeCount;
    size_t edgeCount;
    size_t walkEdges;
    size_t busEdges;
    size_t metroEdges;
    double minLat, maxLat, minLon, maxLon;
    double totalRoadLength;  // kilometers
    
    GraphStats() : nodeCount(0), edgeCount(0), walkEdges(0), 
                   busEdges(0), metroEdges(0), minLat(90), maxLat(-90),
                   minLon(180), maxLon(-180), totalRoadLength(0) {}
};

// Graph class with enhanced features
class Graph {
private:
    // Cache for frequently accessed nodes
    mutable unordered_map<uint32_t, vector<uint32_t>> spatialCache;
    
    void updateStats(const Node& node);
    void updateStats(const Edge& edge);

public:
    vector<Node> nodes;
    vector<vector<Edge>> adj;  // adjacency list
    vector<vector<uint32_t>> transferNodes;  // Nodes where mode transfer possible
    GraphStats stats;
    
    Graph() {
        nodes.reserve(100000);    // Reserve for 100k nodes
        adj.reserve(100000);
        transferNodes.resize(5);  // One vector per mode type
    }
    
    // Node management
    uint32_t addNode(float lat, float lon, const string& name = "");
    bool removeNode(uint32_t id);  // Careful with this!
    
    // Edge management
    void addEdge(uint32_t from, uint32_t to, float distance, 
                 float travel_time, TransportMode mode, bool bidirectional = true);
    
    // Add edge with speed (km/h) instead of time
    void addEdgeWithSpeed(uint32_t from, uint32_t to, float distance,
                          float speedKmph, TransportMode mode, bool bidirectional = true);
    
    // Multi-modal support
    bool canTransfer(uint32_t nodeId, TransportMode mode1, TransportMode mode2) const;
    vector<uint32_t> findTransferPoints(TransportMode fromMode, TransportMode toMode) const;
    
    // Query methods
    const Node& getNode(uint32_t id) const { return nodes[id]; }
    const vector<Edge>& getEdges(uint32_t id) const { return adj[id]; }
    
    // Spatial queries
    vector<uint32_t> findNodesInRadius(float lat, float lon, float radiusMeters) const;
    uint32_t findNearestNode(float lat, float lon, float maxDistance = 100.0) const;
    
    // Statistics
    const GraphStats& getStats() const { return stats; }
    void printStats() const;
    
    // Clear and reset
    void clear();
    
    // Serialization
    bool saveToFile(const string& filename) const;
    bool loadFromFile(const string& filename);
};

#endif
