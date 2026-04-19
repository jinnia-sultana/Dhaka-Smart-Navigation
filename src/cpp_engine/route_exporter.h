#ifndef ROUTE_EXPORTER_H
#define ROUTE_EXPORTER_H

#include "graph.h"
#include "routing.h"
#include <string>

class RouteExporter {
public:
    static void exportToJSON(const Graph& graph, const RouteResult& route, const std::string& filename);
};

#endif