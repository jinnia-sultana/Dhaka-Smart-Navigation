#include "nodesAndEdges.h"
#include "mode.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <unordered_map>
#include <string>
#include <set>
#include<numeric>

using namespace std;

//Globals
vector<Node>        nodes;
vector<Edge>        edges;
vector<vector<int>> adj;
int numNodes = 0;
int numEdges = 0;

//Internal dedup map: "lat6,lon6" -> node id
static unordered_map<string, int> coordMap;
static unordered_map<string, int> nameMap;   //named stop dedup

static string coordKey(double lat, double lon) {
    char buf[48];
    snprintf(buf, sizeof(buf), "%.6f,%.6f", lat, lon);
    return string(buf);
}


int findOrAddNode(double lat, double lon, const char* name) {
    //check if a node with this name already exists
    if (name && name[0]) {
        string nameStr(name);
        auto it = nameMap.find(nameStr);
        if (it != nameMap.end()) return it->second;
    }

    // Check by coordinate
    string key = coordKey(lat, lon);
    auto it = coordMap.find(key);
    if (it != coordMap.end()) {
        // If this coordinate gets a name now, save it
        if (name && name[0] && nodes[it->second].name[0] == '\0') {
            strncpy(nodes[it->second].name, name, 63);
            nodes[it->second].name[63] = '\0';
            nameMap[string(name)] = it->second;
            printf("  Named node %d as '%s'\n", it->second, name);
        }
        return it->second;
    }

    // Create new node
    Node n;
    n.id = numNodes;
    n.lat = lat;
    n.lon = lon;
    if (name && name[0]) {
        strncpy(n.name, name, 63);
        n.name[63] = '\0';
        nameMap[string(name)] = numNodes;
        printf("  Created new node %d with name '%s'\n", numNodes, name);
    } else {
        n.name[0] = '\0';
    }
    nodes.push_back(n);
    adj.push_back(vector<int>());
    coordMap[key] = numNodes;
    return numNodes++;
}

int findNearestNode(double lat, double lon) {
    int best = -1;
    double bestDist = 1e18;
    for (int i = 0; i < numNodes; i++) {
        double d = haversineDistance(lat, lon, nodes[i].lat, nodes[i].lon);
        if (d < bestDist) { bestDist = d; best = i; }
    }
    return best;
}


void addEdge(int from, int to, Mode mode, double distance) {
    Edge e;
    e.from = from; e.to = to; e.mode = mode; e.distance = distance;
    edges.push_back(e);
    adj[from].push_back(numEdges);
    numEdges++;
}


double haversineDistance(double lat1, double lon1, double lat2, double lon2) {
    static const double R = 6371000.0;
    static const double PI = 3.14159265358979323846;
    double dlat = (lat2 - lat1) * PI / 180.0;
    double dlon = (lon2 - lon1) * PI / 180.0;
    double a = sin(dlat/2)*sin(dlat/2) +
               cos(lat1*PI/180.0)*cos(lat2*PI/180.0)*sin(dlon/2)*sin(dlon/2);
    return R * 2.0 * atan2(sqrt(a), sqrt(1.0-a));
}

// For every (metro/bus) node, find the nearest STREET node within
// radiusM and add a two-way WALK edge between them.
void connectTransitToStreets(double radiusM) {
    printf("Connecting transit stops to streets (radius %.0f m)...\n", radiusM);
    
    //identify street nodes(WALK or BIKE edges)
    vector<bool> isStreet(numNodes, false);
    for (int i = 0; i < numNodes; i++) {
        for (int eidx : adj[i]) {
            Mode m = edges[eidx].mode;
            if (m == MODE_WALK || m == MODE_BIKE) {
                isStreet[i] = true;
                break;
            }
        }
    }
    
    // Collect transit nodes(METRO or BUS edges)
    vector<int> transitNodes;
    for (int i = 0; i < numNodes; i++) {
        for (int eidx : adj[i]) {
            Mode m = edges[eidx].mode;
            if (m == MODE_METRO || m == MODE_BIKOLPO || m == MODE_UTTARA) {
                transitNodes.push_back(i);
                break;
            }
        }
    }
    
    printf("  Found %d transit nodes, %d street nodes\n", (int)transitNodes.size(), (int)count(isStreet.begin(), isStreet.end(), true));
    
    //For a transit node, find nearest street node, add walking connection
    int connections = 0;
    for (int tid : transitNodes) {
        double bestDist = radiusM;
        int bestStreet = -1;
        
        for (int sid = 0; sid < numNodes; sid++) {
            if (!isStreet[sid]) continue;
            if (sid == tid) continue;
            
            double dist = haversineDistance(nodes[tid].lat, nodes[tid].lon,
                                             nodes[sid].lat, nodes[sid].lon);
            if (dist < bestDist) {
                bestDist = dist;
                bestStreet = sid;
            }
        }
        
        if (bestStreet != -1) {
            addEdge(tid, bestStreet, MODE_WALK, bestDist);
            addEdge(bestStreet, tid, MODE_WALK, bestDist);
            connections += 2;
        }
    }
    
    printf("  Added %d walking connections between transit and streets\n", connections);
}