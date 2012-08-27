#ifndef _GRAPH_H_
#define _GRAPH_H_

#define GRAPH_METATABLE   "leda read-only graph"
#include <lua.h>

#include "extra/threading.h"

#define STAGE(i) main_graph->s[i]
#define CONNECTOR(i) main_graph->c[i]
#define CLUSTER(i) main_graph->cl[i]
#define DAEMON(i) main_graph->d[i]

typedef size_t stage_id;
typedef size_t connector_id;
typedef size_t cluster_id;
typedef size_t daemon_id;


typedef struct {
   union {
      char const * c;
      lua_Number n;
   } key;
   enum {_STRING,_NUMBER} type;
   connector_id value;
} key;

/* leda stage structure */ 
typedef struct stage_data {
   char const * name;
   char const * handler;
   char const * init;
   size_t name_len;
   size_t init_len;
   size_t handler_len;
   key * output;
   size_t n_out;
   bool_t serial;
   int unique_id;
   cluster_id cluster;
} * stage;

typedef struct connector_data {
   char const * name;
   size_t name_len;
   stage_id p;
   stage_id c;
   char const * send;
   size_t send_len;
   int unique_id;
} * connector;

typedef struct daemon_data {
   char const * host;
   size_t host_len;
   int port;
} * leda_daemon;

typedef struct cluster_data {
   char const * name;
   size_t name_len;
   daemon_id * daemons;
   size_t n_daemons;
   bool_t local;
   int unique_id;
} * cluster;

//graph read only representation (thread-safe)
typedef struct graph_data {
   stage * s;
   size_t n_s;
   connector * c;
   size_t n_c;
   cluster * cl;
   size_t n_cl;   
   leda_daemon * d;
   size_t n_d;
   char const * name;
   size_t name_len;
} * graph;

extern graph main_graph;

int graph_createmetatable (lua_State *L);
graph to_graph(lua_State *L,int i);
int graph_build(lua_State * L);

#endif //_GRAPH_H_
