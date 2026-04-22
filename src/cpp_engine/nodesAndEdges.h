#ifndef NODES_AND_EDGES_H
#define NODES_AND_EDGES_H

#include "mode.h"
#include <vector>
#include <string>

using namespace std;

#define INF 1e18


struct Edge {
    int   from;
    int   to;
    Mode  mode;
    double distance;   // metres
};


struct Node {
    int    id;
    char   name[64];
    double lat;
    double lon;
};


extern vector<Node>          nodes;
extern vector<Edge>          edges;
extern vector<vector<int>>   adj;     // adj[u] = list of edge indices
extern int numNodes;
extern int numEdges;


int    findOrAddNode(double lat, double lon, const char* name = nullptr);
int    findNearestNode(double lat, double lon);
void   addEdge(int from, int to, Mode mode, double distance);
double haversineDistance(double lat1, double lon1, double lat2, double lon2);

//Connect within `radiusM` metres
void   connectTransitToStreets(double radiusM = 300.0);

#endif