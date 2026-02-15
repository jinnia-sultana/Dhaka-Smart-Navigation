#include <wx/wx.h>
#include <wx/slider.h>
#include <wx/combobox.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/msgdlg.h>

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <queue>
#include <algorithm>
#include <cmath>

using namespace std;

//cd src
// (.\buildwx.bat main)to run the code

// ===================== ROUTING ENGINE ======================

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
    Edge(int f, int t, double d, string m) : from(f), to(t), distance(d), mode(m) {}
};

struct RouteOption {
    string mode;
    vector<int> path;
    double distance;
    double time;
    double cost;
    double score;
    bool operator<(const RouteOption& o) const { return score > o.score; }
};

vector<Node> nodes;
vector<Edge> edges;
map<int, vector<int>> adj;

void initializeGraph() {
    nodes = {
        {0,"Dhanmondi",10,50},{1,"Farmgate",30,80},{2,"Shahbag",50,50},
        {3,"Motijheel",80,20},{4,"Gulistan",70,30},{5,"Kalabagan",20,40},
        {6,"KarwanBazar",40,60},{7,"PressClub",60,40}
    };

    auto add = [&](int a,int b,double d,string m){
        edges.push_back({a,b,d,m});
        edges.push_back({b,a,d,m});
    };

    add(0,1,5,"walk"); add(1,2,5,"walk"); add(2,3,5,"walk");
    add(3,4,5,"walk"); add(0,5,3,"walk"); add(5,6,3,"walk");
    add(6,7,3,"walk"); add(7,2,3,"walk");

    add(0,1,5,"rickshaw"); add(1,2,5,"rickshaw"); add(2,3,5,"rickshaw");
    add(3,4,5,"rickshaw"); add(0,2,12,"rickshaw"); add(1,3,12,"rickshaw");

    add(0,2,12,"bus"); add(2,4,12,"bus"); add(0,6,8,"bus");
    add(6,3,10,"bus"); add(3,4,5,"bus");

    adj.clear();
    for(int i=0;i<edges.size();i++)
        adj[edges[i].from].push_back(i);
}

double heuristic(int a,int b){
    double dx=nodes[a].x-nodes[b].x;
    double dy=nodes[a].y-nodes[b].y;
    return sqrt(dx*dx+dy*dy);
}

vector<int> astar(int start,int goal,string mode){
    priority_queue<pair<double,int>,vector<pair<double,int>>,greater<>> pq;
    map<int,double> g;
    map<int,int> parent;

    pq.push({0,start});
    g[start]=0;

    while(!pq.empty()){
        int u=pq.top().second; pq.pop();
        if(u==goal) break;

        for(int ei: adj[u]){
            Edge &e=edges[ei];
            if(e.mode!=mode) continue;
            double ng=g[u]+e.distance;
            if(!g.count(e.to)||ng<g[e.to]){
                g[e.to]=ng;
                parent[e.to]=u;
                pq.push({ng+heuristic(e.to,goal),e.to});
            }
        }
    }

    vector<int> path;
    if(!parent.count(goal)&&start!=goal) return path;
    for(int v=goal;v!=start;v=parent[v]) path.push_back(v);
    path.push_back(start);
    reverse(path.begin(),path.end());
    return path;
}

double timeCalc(string m,double d){
    if(m=="walk") return d/5*60;
    if(m=="rickshaw") return d/15*60;
    return d/25*60;
}
double costCalc(string m,double d,int seg){
    if(m=="walk") return 0;
    if(m=="rickshaw") return d*30;
    return seg*5;
}

vector<RouteOption> calculateRoutes(string from,string to,int tw,int cw,int dw){
    map<string,int> idx;
    for(auto &n:nodes) idx[n.name]=n.id;

    vector<RouteOption> res;
    for(string m: {"walk","rickshaw","bus"}){
        auto path=astar(idx[from],idx[to],m);
        if(path.empty()) continue;
        double dist=0;
        for(int i=0;i+1<path.size();i++)
            for(auto &e:edges)
                if(e.from==path[i]&&e.to==path[i+1]&&e.mode==m)
                    dist+=e.distance;

        double t=timeCalc(m,dist);
        double c=costCalc(m,dist,path.size()-1);
        double score=1000/(1+t*tw/100+c*cw/50+dist*dw);

        res.push_back({m,path,dist,t,c,score});
    }
    sort(res.begin(),res.end());
    return res;
}

// ===================== WXWIDGETS UI ======================

class MainFrame : public wxFrame {
public:
    wxComboBox *fromCombo, *toCombo;
    wxSlider *timeSlider, *costSlider, *distSlider;
    wxTextCtrl *outputText;
    
    MainFrame() : wxFrame(NULL, wxID_ANY, "Navigation System", wxDefaultPosition, wxSize(800, 500)) {
        initializeGraph();
        
        wxPanel *panel = new wxPanel(this, wxID_ANY);
        wxBoxSizer *mainSizer = new wxBoxSizer(wxHORIZONTAL);
        
        // Left panel (controls)
        wxPanel *leftPanel = new wxPanel(panel, wxID_ANY);
        wxBoxSizer *leftSizer = new wxBoxSizer(wxVERTICAL);
        
        wxArrayString locations;
        locations.Add("Dhanmondi");
        locations.Add("Farmgate");
        locations.Add("Shahbag");
        locations.Add("Motijheel");
        locations.Add("Gulistan");
        locations.Add("Kalabagan");
        locations.Add("KarwanBazar");
        locations.Add("PressClub");
        
        leftSizer->Add(new wxStaticText(leftPanel, wxID_ANY, "From:"), 0, wxALL, 5);
        fromCombo = new wxComboBox(leftPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, locations, wxCB_DROPDOWN);
        leftSizer->Add(fromCombo, 0, wxEXPAND | wxALL, 5);
        
        leftSizer->Add(new wxStaticText(leftPanel, wxID_ANY, "To:"), 0, wxALL, 5);
        toCombo = new wxComboBox(leftPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, locations, wxCB_DROPDOWN);
        leftSizer->Add(toCombo, 0, wxEXPAND | wxALL, 5);
        
        leftSizer->Add(new wxStaticText(leftPanel, wxID_ANY, "Time Weight:"), 0, wxALL, 5);
        timeSlider = new wxSlider(leftPanel, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
        leftSizer->Add(timeSlider, 0, wxEXPAND | wxALL, 5);
        
        leftSizer->Add(new wxStaticText(leftPanel, wxID_ANY, "Cost Weight:"), 0, wxALL, 5);
        costSlider = new wxSlider(leftPanel, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
        leftSizer->Add(costSlider, 0, wxEXPAND | wxALL, 5);
        
        leftSizer->Add(new wxStaticText(leftPanel, wxID_ANY, "Distance Weight:"), 0, wxALL, 5);
        distSlider = new wxSlider(leftPanel, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
        leftSizer->Add(distSlider, 0, wxEXPAND | wxALL, 5);
        
        wxButton *findButton = new wxButton(leftPanel, wxID_ANY, "Find Route");
        leftSizer->Add(findButton, 0, wxEXPAND | wxALL, 15);
        
        leftPanel->SetSizer(leftSizer);
        
        // Right panel (output)
        wxPanel *rightPanel = new wxPanel(panel, wxID_ANY);
        wxBoxSizer *rightSizer = new wxBoxSizer(wxVERTICAL);
        
        rightSizer->Add(new wxStaticText(rightPanel, wxID_ANY, "Results:"), 0, wxALL, 5);
        outputText = new wxTextCtrl(rightPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);
        rightSizer->Add(outputText, 1, wxEXPAND | wxALL, 5);
        
        rightPanel->SetSizer(rightSizer);
        
        // Add panels to main sizer
        mainSizer->Add(leftPanel, 1, wxEXPAND | wxALL, 10);
        mainSizer->Add(rightPanel, 2, wxEXPAND | wxALL, 10);
        
        panel->SetSizer(mainSizer);
        
        // Bind button event
        findButton->Bind(wxEVT_BUTTON, &MainFrame::OnFindRoute, this);
        
        // Set defaults
        fromCombo->SetSelection(0);
        toCombo->SetSelection(1);
    }
    
    void OnFindRoute(wxCommandEvent& event) {
    string from = string(fromCombo->GetValue().mb_str());
    string to = string(toCombo->GetValue().mb_str());
    
    if (from.empty() || to.empty()) {
        wxMessageBox("Please select both From and To locations", "Error", wxOK | wxICON_ERROR);
        return;
    }
    
    auto routes = calculateRoutes(from, to, timeSlider->GetValue(), costSlider->GetValue(), distSlider->GetValue());
    
    outputText->Clear();
    if (routes.empty()) {
        outputText->AppendText("No routes found!\n");
        return;
    }
    
    // Display with ranking
    int rank = 1;
    for (auto &r : routes) {
        outputText->AppendText(wxString::Format("=== ROUTE #%d (Score: %.2f) ===\n", rank, r.score));
        outputText->AppendText(wxString::Format("Mode: %s\n", r.mode));
        outputText->AppendText("Path: ");
        for (int i = 0; i < r.path.size(); i++) {
            outputText->AppendText(nodes[r.path[i]].name);
            if (i != r.path.size() - 1) outputText->AppendText(" → ");
        }
        outputText->AppendText("\n");
        outputText->AppendText(wxString::Format("Distance: %.1f km\n", r.distance));
        outputText->AppendText(wxString::Format("Time: %.1f min\n", r.time));
        outputText->AppendText(wxString::Format("Cost: %.1f Tk\n", r.cost));
        outputText->AppendText("\n");
        rank++;
    }
}
};

class MyApp : public wxApp {
public:
    virtual bool OnInit() {
        MainFrame *frame = new MainFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(MyApp);
