

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  #pragma comment(lib, "ws2_32.lib")
#else
  #include <unistd.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
#endif

#ifdef __EMSCRIPTEN__
  #include <emscripten.h>
  #define EXPORT EMSCRIPTEN_KEEPALIVE
#else
  #define EXPORT
#endif

#define MAX_NODES 128
#define MAX_EDGES 512
#define INF 1000000000
#define PI 3.14159265358979323846


typedef struct {
    char id[16];
    char label[64];
    char cat[16];
    int x;
    int y;
} Node;

typedef struct AdjEdge {
    int target;
    int weight;
    struct AdjEdge* next;
} AdjEdge;

typedef struct {
    AdjEdge* head;
} AdjList;

typedef struct {
    int node_id;
    int dist;
} HeapElement;

typedef struct {
    HeapElement data[MAX_NODES * 8];
    int size;
} MinHeap;

static Node g_nodes[MAX_NODES];
static int g_node_count = 0;
static AdjList g_adj[MAX_NODES];


 // MIN-HEAP PRIORITY QUEUE IMPLEMENTATION

static void heap_init(MinHeap* h) {
    h->size = 0;
}

static void heap_swap(HeapElement* a, HeapElement* b) {
    HeapElement tmp = *a;
    *a = *b;
    *b = tmp;
}

static void heap_push(MinHeap* h, int node_id, int dist) {
    int i = h->size++;
    h->data[i].node_id = node_id;
    h->data[i].dist = dist;

    while (i > 0) {
        int parent = (i - 1) / 2;
        if (h->data[parent].dist <= h->data[i].dist) break;
        heap_swap(&h->data[parent], &h->data[i]);
        i = parent;
    }
}

static HeapElement heap_pop(MinHeap* h) {
    HeapElement top = h->data[0];
    h->data[0] = h->data[--h->size];

    int i = 0;
    while (1) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = i;

        if (left < h->size && h->data[left].dist < h->data[smallest].dist)
            smallest = left;
        if (right < h->size && h->data[right].dist < h->data[smallest].dist)
            smallest = right;
        if (smallest == i) break;

        heap_swap(&h->data[i], &h->data[smallest]);
        i = smallest;
    }
    return top;
}

static int find_node_index(const char* id) {
    for (int i = 0; i < g_node_count; ++i) {
        if (strcmp(g_nodes[i].id, id) == 0) return i;
    }
    return -1;
}

static int add_node(const char* id, const char* label, const char* cat, int x, int y) {
    int idx = find_node_index(id);
    if (idx != -1) return idx;

    idx = g_node_count++;
    strncpy(g_nodes[idx].id, id, sizeof(g_nodes[idx].id) - 1);
    strncpy(g_nodes[idx].label, label, sizeof(g_nodes[idx].label) - 1);
    strncpy(g_nodes[idx].cat, cat, sizeof(g_nodes[idx].cat) - 1);
    g_nodes[idx].x = x;
    g_nodes[idx].y = y;
    g_adj[idx].head = NULL;
    return idx;
}

static void add_edge(const char* u_id, const char* v_id, int weight) {
    int u = find_node_index(u_id);
    int v = find_node_index(v_id);
    if (u < 0 || v < 0) return;

    // Add u -> v
    AdjEdge* e1 = (AdjEdge*)malloc(sizeof(AdjEdge));
    e1->target = v;
    e1->weight = weight;
    e1->next = g_adj[u].head;
    g_adj[u].head = e1;

    // Add v -> u (undirected graph)
    AdjEdge* e2 = (AdjEdge*)malloc(sizeof(AdjEdge));
    e2->target = u;
    e2->weight = weight;
    e2->next = g_adj[v].head;
    g_adj[v].head = e2;
}

//map data
void init_campus_graph(void) {
    g_node_count = 0;

    // Core Academic
    add_node("HLC", "Hendricks Leadership Center", "academic", 358, 388);
    add_node("JW", "JW Building", "academic", 383, 310);
    add_node("TSF", "TSF Building", "academic", 415, 255);
    add_node("BW", "BW Building", "academic", 276, 385);
    add_node("FH", "FH Building", "academic", 510, 448);
    add_node("CS", "CS Building", "academic", 597, 344);
    add_node("WA", "WA Hall", "academic", 335, 540);
    add_node("JM", "JM Hall", "academic", 437, 550);
    add_node("ATC", "ATC Building", "academic", 843, 710);
    add_node("HCDC", "Health & Counseling Center", "service", 737, 712);
    add_node("JP", "JP Building", "academic", 651, 130);

    // Residence
    add_node("GP", "Grace Place", "residence", 207, 510);
    add_node("GH", "Gleason Hall", "residence", 527, 668);
    add_node("MH", "Metz Hall", "residence", 577, 748);
    add_node("JETS", "JETS Hall", "residence", 648, 808);
    add_node("CV", "Central Village", "residence", 337, 845);
    add_node("JAS", "JAS Hall", "residence", 615, 460);
    add_node("EIC", "EIC Building", "residence", 695, 486);
    add_node("AO", "AO Building", "service", 737, 558);

    // Jets Village
    add_node("JVA", "Jets Village A", "residence", 662, 598);
    add_node("JVB", "Jets Village B", "residence", 648, 583);
    add_node("JVC", "Jets Village C", "residence", 658, 628);
    add_node("JVD", "Jets Village D", "residence", 631, 578);
    add_node("JVE", "Jets Village E", "residence", 645, 613);
    add_node("JVF", "Jets Village F", "residence", 629, 613);

    // Parking
    add_node("P1", "Parking Lot P1", "parking", 513, 358);
    add_node("P2", "Parking Lot P2", "parking", 460, 348);
    add_node("P3", "Parking Lot P3", "parking", 502, 308);
    add_node("P4", "Parking Lot P4", "parking", 491, 263);
    add_node("G1", "Parking Lot G1", "parking", 313, 660);
    add_node("G2", "Parking Lot G2", "parking", 313, 708);
    add_node("H1", "Parking Lot H1", "parking", 398, 622);
    add_node("H2", "Parking Lot H2", "parking", 398, 672);
    add_node("H3", "Parking Lot H3", "parking", 398, 706);
    add_node("H4", "Parking Lot H4", "parking", 400, 740);
    add_node("J1", "Parking Lot J1", "parking", 634, 808);
    add_node("J2", "Parking Lot J2", "parking", 581, 800);
    add_node("J3", "Parking Lot J3", "parking", 513, 826);
    add_node("K", "Parking Lot K", "parking", 644, 757);
    add_node("L", "Parking Lot L", "parking", 744, 574);
    add_node("M1", "Parking Lot M1", "parking", 711, 463);
    add_node("M2", "Parking Lot M2", "parking", 711, 493);
    add_node("N", "Parking Lot N", "parking", 609, 370);

    // Sports & Recreation
    add_node("SoccerField", "Soccer Field", "sports", 852, 375);
    add_node("SoftballField", "Softball Field", "sports", 802, 112);
    add_node("BaseballField", "Baseball Field", "sports", 775, 205);
    add_node("DiscGolf", "Disc Golf Course", "sports", 620, 893);
    add_node("Basketball", "Outdoor Basketball", "sports", 722, 820);
    add_node("Volleyball", "Sand Volleyball", "sports", 720, 844);

    // Landmarks
    add_node("ZennethPond", "Zenneth Pond", "landmark", 164, 148);
    add_node("FlagMemorial", "Flag Memorial", "landmark", 270, 152);
    add_node("WaterTower", "Water Tower", "landmark", 862, 793);

    // Path Junctions
    add_node("INT_A", "Junction (Jets Trail W)", "junction", 418, 200);
    add_node("INT_B", "Junction (Jets Trail E)", "junction", 570, 200);
    add_node("INT_C", "Junction (Central)", "junction", 410, 480);
    add_node("INT_D", "Junction (East-Central)", "junction", 550, 480);
    add_node("INT_E", "Junction (East Loop)", "junction", 650, 480);
    add_node("INT_F", "Junction (SE Loop)", "junction", 710, 600);
    add_node("INT_G", "Junction (South-Central)", "junction", 490, 600);
    add_node("INT_H", "Junction (SW)", "junction", 350, 750);
    add_node("INT_I", "Junction (South)", "junction", 490, 750);
    add_node("INT_J", "Junction (West Loop)", "junction", 220, 380);
    add_node("INT_K", "Junction (West-Mid)", "junction", 220, 550);
    add_node("INT_L", "Junction (East Road)", "junction", 750, 380);
    add_node("INT_M", "Junction (SE Corner)", "junction", 750, 700);

    // Edges
    add_edge("INT_A", "INT_B", 152);
    add_edge("INT_A", "TSF", 60);
    add_edge("INT_A", "JW", 105);
    add_edge("INT_A", "FlagMemorial", 120);
    add_edge("INT_B", "JP", 90);
    add_edge("INT_B", "CS", 140);
    add_edge("INT_B", "P3", 80);
    add_edge("FlagMemorial", "ZennethPond", 110);
    add_edge("ZennethPond", "INT_J", 240);

    add_edge("TSF", "JW", 58);
    add_edge("JW", "P2", 50);
    add_edge("JW", "P3", 75);
    add_edge("JW", "P4", 70);
    add_edge("P4", "INT_B", 80);
    add_edge("P3", "INT_B", 85);
    add_edge("TSF", "P4", 55);

    add_edge("JW", "HLC", 90);
    add_edge("HLC", "BW", 80);
    add_edge("HLC", "FH", 100);
    add_edge("HLC", "WA", 100);
    add_edge("HLC", "INT_C", 60);
    add_edge("BW", "INT_J", 90);
    add_edge("BW", "GP", 120);
    add_edge("FH", "JAS", 105);
    add_edge("FH", "P1", 55);
    add_edge("FH", "P2", 65);
    add_edge("P1", "CS", 100);
    add_edge("P1", "N", 50);
    add_edge("P2", "INT_C", 70);
    add_edge("CS", "N", 55);
    add_edge("N", "INT_D", 60);

    add_edge("CS", "INT_L", 165);
    add_edge("JAS", "EIC", 90);
    add_edge("JAS", "INT_D", 80);
    add_edge("EIC", "INT_E", 55);
    add_edge("EIC", "M1", 30);
    add_edge("EIC", "M2", 45);
    add_edge("INT_E", "INT_L", 65);
    add_edge("INT_L", "SoccerField", 110);
    add_edge("INT_L", "BaseballField", 185);
    add_edge("BaseballField", "SoftballField", 95);
    add_edge("SoftballField", "JP", 120);
    add_edge("JP", "INT_B", 180);

    add_edge("INT_J", "INT_K", 170);
    add_edge("INT_J", "BW", 90);
    add_edge("INT_K", "GP", 100);
    add_edge("GP", "WA", 140);
    add_edge("INT_K", "G1", 120);
    add_edge("G1", "G2", 50);

    add_edge("WA", "JM", 100);
    add_edge("JM", "INT_G", 85);
    add_edge("WA", "INT_C", 85);
    add_edge("INT_C", "H1", 65);
    add_edge("JM", "H1", 70);
    add_edge("H1", "H2", 52);
    add_edge("H2", "H3", 40);
    add_edge("H3", "H4", 42);
    add_edge("H4", "INT_H", 80);
    add_edge("INT_H", "CV", 70);
    add_edge("CV", "J3", 110);
    add_edge("J3", "INT_I", 80);

    add_edge("INT_G", "GH", 75);
    add_edge("GH", "MH", 90);
    add_edge("MH", "JETS", 75);
    add_edge("JETS", "J1", 60);
    add_edge("J1", "K", 55);
    add_edge("K", "HCDC", 95);
    add_edge("HCDC", "ATC", 110);
    add_edge("ATC", "WaterTower", 60);
    add_edge("ATC", "INT_M", 60);
    add_edge("INT_M", "INT_F", 105);
    add_edge("INT_F", "AO", 80);
    add_edge("AO", "L", 30);
    add_edge("INT_F", "INT_E", 155);
    add_edge("INT_F", "EIC", 180);

    add_edge("JVA", "JVB", 28);
    add_edge("JVB", "JVD", 22);
    add_edge("JVC", "JVE", 20);
    add_edge("JVE", "JVF", 18);
    add_edge("JVD", "JVF", 22);
    add_edge("JVA", "JVC", 30);
    add_edge("JVB", "INT_G", 80);
    add_edge("JVD", "INT_G", 85);
    add_edge("JVF", "INT_G", 70);
    add_edge("JVC", "GH", 65);
    add_edge("JVA", "AO", 115);

    add_edge("G2", "INT_H", 70);
    add_edge("INT_H", "H4", 80);
    add_edge("J2", "JETS", 50);
    add_edge("J2", "J3", 70);
    add_edge("J1", "JETS", 62);

    add_edge("DiscGolf", "INT_I", 130);
    add_edge("DiscGolf", "J3", 160);
    add_edge("Basketball", "JETS", 80);
    add_edge("Volleyball", "Basketball", 30);
    add_edge("Basketball", "K", 80);

    add_edge("INT_D", "INT_C", 145);
    add_edge("INT_D", "INT_G", 130);
    add_edge("INT_G", "INT_I", 165);
    add_edge("INT_I", "INT_H", 170);
    add_edge("INT_K", "INT_J", 170);
    add_edge("L", "INT_F", 55);
    add_edge("M1", "INT_L", 45);
    add_edge("M2", "INT_L", 55);
}

 //DIJKSTRA'S SHORTEST PATH ALGORITHM IN C

EXPORT int dijkstra(int src_idx, int dst_idx, int* out_path, int* out_dist) {
    if (src_idx < 0 || src_idx >= g_node_count || dst_idx < 0 || dst_idx >= g_node_count) {
        return 0;
    }

    int dist[MAX_NODES];
    int prev[MAX_NODES];
    int visited[MAX_NODES];

    for (int i = 0; i < g_node_count; ++i) {
        dist[i] = INF;
        prev[i] = -1;
        visited[i] = 0;
    }

    MinHeap pq;
    heap_init(&pq);

    dist[src_idx] = 0;
    heap_push(&pq, src_idx, 0);

    while (pq.size > 0) {
        HeapElement cur = heap_pop(&pq);
        int u = cur.node_id;

        if (visited[u]) continue;
        visited[u] = 1;

        if (u == dst_idx) break;

        for (AdjEdge* e = g_adj[u].head; e != NULL; e = e->next) {
            int v = e->target;
            int weight = e->weight;

            if (!visited[v] && dist[u] + weight < dist[v]) {
                dist[v] = dist[u] + weight;
                prev[v] = u;
                heap_push(&pq, v, dist[v]);
            }
        }
    }

    if (dist[dst_idx] >= INF) {
        *out_dist = -1;
        return 0;
    }

    *out_dist = dist[dst_idx];

    // Backtrack path
    int temp_path[MAX_NODES];
    int length = 0;
    int curr = dst_idx;

    while (curr != -1) {
        temp_path[length++] = curr;
        curr = prev[curr];
    }

    // Reverse into out_path
    for (int i = 0; i < length; ++i) {
        out_path[i] = temp_path[length - 1 - i];
    }

    return length;
}

static const char* calculate_bearing(int x1, int y1, int x2, int y2) {
    double dx = (double)(x2 - x1);
    double dy = (double)(y2 - y1);
    double angle = atan2(dy, dx) * 180.0 / PI;

    if (angle > -22.5 && angle <= 22.5) return "East";
    if (angle > 22.5 && angle <= 67.5) return "Southeast";
    if (angle > 67.5 && angle <= 112.5) return "South";
    if (angle > 112.5 && angle <= 157.5) return "Southwest";
    if (angle > 157.5 || angle <= -157.5) return "West";
    if (angle > -157.5 && angle <= -112.5) return "Northwest";
    if (angle > -112.5 && angle <= -67.5) return "North";
    return "Northeast";
}

EXPORT int get_route_json(const char* src_id, const char* dst_id, char* out_buf, int max_len) {
    int src = find_node_index(src_id);
    int dst = find_node_index(dst_id);

    if (src < 0 || dst < 0) {
        snprintf(out_buf, max_len, "{\"found\":0,\"error\":\"Invalid node ID\"}");
        return 0;
    }

    int path[MAX_NODES];
    int total_dist = 0;
    int count = dijkstra(src, dst, path, &total_dist);

    if (count <= 0) {
        snprintf(out_buf, max_len, "{\"found\":0,\"error\":\"No path found\"}");
        return 0;
    }

    int offset = snprintf(out_buf, max_len,
        "{\"found\":1,\"from\":\"%s\",\"to\":\"%s\",\"distance\":%d,\"count\":%d,\"path\":[",
        src_id, dst_id, total_dist, count);

    for (int i = 0; i < count; ++i) {
        offset += snprintf(out_buf + offset, max_len - offset,
            "\"%s\"%s", g_nodes[path[i]].id, (i + 1 < count) ? "," : "");
    }

    offset += snprintf(out_buf + offset, max_len - offset, "],\"coords\":[");

    for (int i = 0; i < count; ++i) {
        int idx = path[i];
        offset += snprintf(out_buf + offset, max_len - offset,
            "{\"id\":\"%s\",\"label\":\"%s\",\"cat\":\"%s\",\"x\":%d,\"y\":%d}%s",
            g_nodes[idx].id, g_nodes[idx].label, g_nodes[idx].cat, g_nodes[idx].x, g_nodes[idx].y,
            (i + 1 < count) ? "," : "");
    }

    offset += snprintf(out_buf + offset, max_len - offset, "],\"steps\":[");

    for (int i = 0; i < count; ++i) {
        int idx = path[i];
        char step_buf[256];
        if (i == 0) {
            snprintf(step_buf, sizeof(step_buf), "Start at %s (%s)", g_nodes[idx].id, g_nodes[idx].label);
        } else {
            int prev_idx = path[i - 1];
            const char* dir = calculate_bearing(g_nodes[prev_idx].x, g_nodes[prev_idx].y, g_nodes[idx].x, g_nodes[idx].y);
            double dx = g_nodes[idx].x - g_nodes[prev_idx].x;
            double dy = g_nodes[idx].y - g_nodes[prev_idx].y;
            int seg_dist = (int)(sqrt(dx * dx + dy * dy) * 0.8);
            if (i == count - 1) {
                snprintf(step_buf, sizeof(step_buf), "Head %s (~%dm) to %s (%s) [Arrive at Destination]", dir, seg_dist, g_nodes[idx].id, g_nodes[idx].label);
            } else {
                snprintf(step_buf, sizeof(step_buf), "Head %s (~%dm) toward %s (%s)", dir, seg_dist, g_nodes[idx].id, g_nodes[idx].label);
            }
        }
        offset += snprintf(out_buf + offset, max_len - offset,
            "\"%s\"%s", step_buf, (i + 1 < count) ? "," : "");
    }

    offset += snprintf(out_buf + offset, max_len - offset, "]}");
    return offset;
}

EXPORT int get_all_nodes_json(char* out_buf, int max_len) {
    int offset = snprintf(out_buf, max_len, "{\"nodes\":[");
    for (int i = 0; i < g_node_count; ++i) {
        offset += snprintf(out_buf + offset, max_len - offset,
            "{\"id\":\"%s\",\"label\":\"%s\",\"cat\":\"%s\",\"x\":%d,\"y\":%d}%s",
            g_nodes[i].id, g_nodes[i].label, g_nodes[i].cat, g_nodes[i].x, g_nodes[i].y,
            (i + 1 < g_node_count) ? "," : "");
    }
    offset += snprintf(out_buf + offset, max_len - offset, "]}");
    return offset;
}

void print_cli_route(const char* src_id, const char* dst_id) {
    int src = find_node_index(src_id);
    int dst = find_node_index(dst_id);

    if (src < 0) {
        printf("Error: Origin '%s' not recognized.\n", src_id);
        return;
    }
    if (dst < 0) {
        printf("Error: Destination '%s' not recognized.\n", dst_id);
        return;
    }

    int path[MAX_NODES];
    int total_dist = 0;
    int count = dijkstra(src, dst, path, &total_dist);

    printf("\n======================================================\n");
    printf("   NAVSYS: CAMPUS NAVIGATION ENGINE (PURE C)\n");
    printf("   Algorithm: Dijkstra Shortest Path | Min-Heap PQ\n");
    printf("======================================================\n");
    printf("Origin:      %s - %s\n", g_nodes[src].id, g_nodes[src].label);
    printf("Destination: %s - %s\n", g_nodes[dst].id, g_nodes[dst].label);

    if (count <= 0) {
        printf("Status:      NO PATH FOUND!\n");
        printf("======================================================\n\n");
        return;
    }

    printf("Distance:    ~%d meters\n", total_dist);
    printf("Waypoints:   %d nodes\n", count);
    printf("Route:       ");
    for (int i = 0; i < count; ++i) {
        printf("%s%s", g_nodes[path[i]].id, (i + 1 < count) ? " -> " : "");
    }
    printf("\n\nTurn-by-turn directions:\n");
    for (int i = 0; i < count; ++i) {
        int idx = path[i];
        if (i == 0) {
            printf("  1. [START] At %s (%s)\n", g_nodes[idx].id, g_nodes[idx].label);
        } else {
            int prev = path[i - 1];
            const char* dir = calculate_bearing(g_nodes[prev].x, g_nodes[prev].y, g_nodes[idx].x, g_nodes[idx].y);
            double dx = g_nodes[idx].x - g_nodes[prev].x;
            double dy = g_nodes[idx].y - g_nodes[prev].y;
            int seg = (int)(sqrt(dx * dx + dy * dy) * 0.8);
            if (i == count - 1) {
                printf("  %d. [ARRIVE] Head %s (~%dm) to %s (%s)\n", i + 1, dir, seg, g_nodes[idx].id, g_nodes[idx].label);
            } else {
                printf("  %d. Head %s (~%dm) to %s (%s)\n", i + 1, dir, seg, g_nodes[idx].id, g_nodes[idx].label);
            }
        }
    }
    printf("======================================================\n\n");
}

void print_cli_node_list(void) {
    printf("\n======================================================================\n");
    printf("   NAVSYS: CAMPUS LOCATION DIRECTORY (%d Nodes Registered in C)\n", g_node_count);
    printf("======================================================================\n");
    printf("%-14s %-32s %-12s %s\n", "CODE", "NAME", "CATEGORY", "COORDINATES");
    printf("----------------------------------------------------------------------\n");
    for (int i = 0; i < g_node_count; ++i) {
        printf("%-14s %-32s %-12s (%d, %d)\n",
               g_nodes[i].id, g_nodes[i].label, g_nodes[i].cat, g_nodes[i].x, g_nodes[i].y);
    }
    printf("======================================================================\n\n");
}

void print_usage(const char* prog) {
    printf("\nNavSys Campus Navigation Engine (Pure C)\n");
    printf("Usage:\n");
    printf("  %s <origin> <destination>      Find shortest path via Dijkstra\n", prog);
    printf("  %s --serve [port]              Start built-in Winsock HTTP server (default: 8000)\n", prog);
    printf("  %s --list                      List all registered campus locations\n", prog);
    printf("  %s --help                      Display this help message\n\n", prog);
    printf("Examples:\n");
    printf("  %s HLC CS\n", prog);
    printf("  %s --serve 8000\n", prog);
    printf("  %s --list\n\n", prog);
}

#ifndef __EMSCRIPTEN__

static void serve_file(SOCKET client, const char* filepath, const char* content_type) {
    FILE* f = fopen(filepath, "rb");
    if (!f) {
        const char* not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 13\r\n\r\n404 Not Found";
        send(client, not_found, (int)strlen(not_found), 0);
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char header[256];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n\r\n", content_type, size);

    send(client, header, hlen, 0);

    char buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        send(client, buf, (int)n, 0);
    }
    fclose(f);
}

static void url_decode(char* dst, const char* src) {
    char a, b;
    while (*src) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a') a -= 'a' - 'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a' - 'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            *dst++ = 16 * a + b;
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

void start_http_server(int port) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize Winsock.\n");
        return;
    }
#endif

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == INVALID_SOCKET) {
        printf("Failed to create socket.\n");
        return;
    }

    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;

    int bound = 0;
    for (int p = port; p < port + 10; ++p) {
        addr.sin_port = htons(p);
        if (bind(server, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            port = p;
            bound = 1;
            break;
        }
    }

    if (!bound) {
        printf("Failed to bind port %d to %d. Is another server running?\n", port, port + 9);
        closesocket(server);
        return;
    }

    if (listen(server, 16) != 0) {
        printf("Failed to listen on socket.\n");
        closesocket(server);
        return;
    }

    printf("\n======================================================\n");
    printf("   NAVSYS C HTTP SERVER STARTED ON PORT %d\n", port);
    printf("   Open in browser: http://localhost:%d\n", port);
    printf("   C Routing API:   http://localhost:%d/api/route?from=HLC&to=CS\n", port);
    printf("   C Source Code:   http://localhost:%d/navsys.c\n", port);
    printf("======================================================\n\n");

    while (1) {
        struct sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        SOCKET client = accept(server, (struct sockaddr*)&client_addr, &client_len);
        if (client == INVALID_SOCKET) continue;

        char req_buf[4096];
        int bytes = recv(client, req_buf, sizeof(req_buf) - 1, 0);
        if (bytes <= 0) {
            closesocket(client);
            continue;
        }
        req_buf[bytes] = '\0';

        char method[16], uri[1024];
        if (sscanf(req_buf, "%15s %1023s", method, uri) != 2) {
            closesocket(client);
            continue;
        }

        printf("[C-Server] %s %s\n", method, uri);

        if (strcmp(uri, "/") == 0 || strcmp(uri, "/index.html") == 0) {
            serve_file(client, "index.html", "text/html; charset=utf-8");
        }
        else if (strcmp(uri, "/campus_map.jpg") == 0) {
            serve_file(client, "campus_map.jpg", "image/jpeg");
        }
        else if (strcmp(uri, "/navsys.c") == 0) {
            serve_file(client, "navsys.c", "text/plain; charset=utf-8");
        }
        else if (strncmp(uri, "/api/route", 10) == 0) {
            char from[64] = {0};
            char to[64] = {0};

            char* query = strchr(uri, '?');
            if (query) {
                char* p_from = strstr(query, "from=");
                if (p_from) {
                    p_from += 5;
                    char* end = strchr(p_from, '&');
                    if (end) {
                        int len = (int)(end - p_from);
                        if (len > 63) len = 63;
                        strncpy(from, p_from, len);
                    } else {
                        strncpy(from, p_from, sizeof(from) - 1);
                    }
                }
                char* p_to = strstr(query, "to=");
                if (p_to) {
                    p_to += 3;
                    char* end = strchr(p_to, '&');
                    if (end) {
                        int len = (int)(end - p_to);
                        if (len > 63) len = 63;
                        strncpy(to, p_to, len);
                    } else {
                        strncpy(to, p_to, sizeof(to) - 1);
                    }
                }
            }

            char dec_from[64], dec_to[64];
            url_decode(dec_from, from);
            url_decode(dec_to, to);

            static char json_buf[32768];
            int jlen = get_route_json(dec_from, dec_to, json_buf, sizeof(json_buf));

            char header[256];
            int hlen = snprintf(header, sizeof(header),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json; charset=utf-8\r\n"
                "Content-Length: %d\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Connection: close\r\n\r\n", jlen);

            send(client, header, hlen, 0);
            send(client, json_buf, jlen, 0);
        }
        else if (strcmp(uri, "/api/nodes") == 0) {
            static char json_buf[32768];
            int jlen = get_all_nodes_json(json_buf, sizeof(json_buf));

            char header[256];
            int hlen = snprintf(header, sizeof(header),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json; charset=utf-8\r\n"
                "Content-Length: %d\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Connection: close\r\n\r\n", jlen);

            send(client, header, hlen, 0);
            send(client, json_buf, jlen, 0);
        }
        else {
            const char* not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
            send(client, not_found, (int)strlen(not_found), 0);
        }

        closesocket(client);
    }

#ifdef _WIN32
    WSACleanup();
#endif
}

#endif /* !__EMSCRIPTEN__ */

int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);
    init_campus_graph();

    if (argc >= 2) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        if (strcmp(argv[1], "--list") == 0 || strcmp(argv[1], "-l") == 0) {
            print_cli_node_list();
            return 0;
        }
    }

    if (argc >= 3 && strcmp(argv[1], "--serve") != 0) {
        // CLI mode: ./navsys <from> <to>
        print_cli_route(argv[1], argv[2]);
        return 0;
    }

#ifndef __EMSCRIPTEN__
    int port = 8000;
    if (argc >= 2 && strcmp(argv[1], "--serve") == 0) {
        if (argc >= 3) {
            port = atoi(argv[2]);
        }
    } else if (argc == 2) {
        // Only one argument provided and not a flag -> show usage
        printf("Error: Missing destination or unknown option '%s'.\n", argv[1]);
        print_usage(argv[0]);
        return 1;
    }

    start_http_server(port);
#else
    printf("NavSys initialized under WebAssembly.\n");
#endif

    return 0;
}
