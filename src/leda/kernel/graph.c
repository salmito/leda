/*
===============================================================================

Copyright (C) 2012 Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

===============================================================================
*/
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <lauxlib.h>
#include <lualib.h>

#include "graph.h"
#include "thread.h"

graph main_graph;

/** get a connector representation from the 'id' defined on graph 'g' */
connector graph_get_connector(graph g,connector_id id) {
   if(!g) return NULL;
   if(id > g->n_c) return NULL;
   return g->c[id];
}

/** get a cluster representation from the 'id' defined on graph 'g' */
cluster graph_get_cluster(graph g,cluster_id id) {
   if(!g) return NULL;
   if(id > g->n_cl) return NULL;
   return g->cl[id];
}


/** get a stage representation from the 'id' defined on graph 'g' */
stage graph_get_stage(graph g,stage_id id) {
   if(!g) return NULL;
   if(id > g->n_s) return NULL;
   return g->s[id];
}

/** Build an internal representation for the graph defined in
 * in the stack on 'index'
 */
graph build_graph_representation(lua_State *L, int index, graph g) {
   int i,j;
   //argument must be a table with a graph definition
   luaL_checktype(L,index, LUA_TTABLE);
   bool_t create_stages_id=FALSE;
   bool_t create_connectors_id=FALSE;
   bool_t create_clusters_id=FALSE;
   bool_t create_processes_id=FALSE;

   lua_getfield(L,index,"stagesid");
   if(lua_type(L,-1)!=LUA_TTABLE) {
      lua_pop(L,1);
      create_stages_id=TRUE;
      lua_newtable(L); //Table to hold stagesid
      lua_pushvalue(L,-1);
      lua_setfield(L,index,"stagesid");
   }
   int stagesid=lua_gettop(L);
   
   lua_getfield(L,index,"connectorsid");
   if(lua_type(L,-1)!=LUA_TTABLE) {
      lua_pop(L,1);
      create_connectors_id=TRUE;
      lua_newtable(L); //Table to hold connectorsid
      lua_pushvalue(L,-1);
      lua_setfield(L,index,"connectorsid");
   } 
   int connectorsid=lua_gettop(L);
   
   lua_getfield(L,index,"clustersid");
   if(lua_type(L,-1)!=LUA_TTABLE) {
      lua_pop(L,1);
      create_clusters_id=TRUE;
      lua_newtable(L); //Table to hold clustersid
      lua_pushvalue(L,-1);
      lua_setfield(L,index,"clustersid");
   } 
   int clustersid=lua_gettop(L);

   lua_getfield(L,index,"processesid");
   if(lua_type(L,-1)!=LUA_TTABLE) {
      lua_pop(L,1);
      create_processes_id=TRUE;
      lua_newtable(L); //Table to hold clustersid
      lua_pushvalue(L,-1);
      lua_setfield(L,index,"processesid");
   } 
   int processesid=lua_gettop(L);

   //struct graph_data * g=calloc(1,sizeof(struct graph_data));
   char const * str;
   size_t len;
   
   lua_getfield (L, index, "name"); //push the name field of stage
   str=lua_tolstring(L, -1, &len); //verify if it's a string
   char * gname=malloc(len+1);
   memcpy(gname,str,len);
   gname[len]='\0';
   g->name=gname;
   g->name_len=len;
   lua_pop(L,1); //pop the name field
   //first, iterate through the processes
   lua_getfield(L, index, "count_processes");
   luaL_checktype(L, -1, LUA_TFUNCTION);
   lua_pushvalue(L,index);
   lua_call(L,1,1);
   luaL_checktype(L, -1, LUA_TNUMBER);
   g->n_d=lua_tointeger(L,-1);
   lua_pop(L,1); //remove number_of_processes
   
   g->d=calloc(g->n_d,sizeof(leda_process));
   lua_getfield(L,index,"processes");
   luaL_checktype(L, -1, LUA_TFUNCTION);
   lua_pushvalue(L,index);
   lua_call(L,1,1);
   luaL_checktype(L, -1, LUA_TTABLE);
   lua_pushnil(L);
   i=0;

   while (lua_next(L, -2) != 0) {
      leda_process d=calloc(1,sizeof(struct process_data));
      lua_pop(L,1);
      lua_getfield(L,-1,"host"); //push cluster.process[j].host
      str=lua_tolstring(L, -1, &len);
      char * dhost=malloc(len+1);
      memcpy(dhost,str,len);
      dhost[len]='\0';
      d->host_len=len;
      d->host=dhost;
      lua_pop(L,1); //pop cluster.process[j].host

      lua_getfield(L,-1,"port"); //push cluster.process[j].port
      d->port=lua_tointeger(L,-1);
      lua_pop(L,1); //pop cluster.process[j].port
      if(create_processes_id) {
         lua_pushvalue(L,-1);
         lua_pushinteger(L,i);
         lua_settable(L,processesid);
         g->d[i++]=d;
       } else {
         lua_pushvalue(L,-1);
         lua_gettable(L,processesid);
         int idx=lua_tointeger(L,-1);
         g->d[idx]=d;
         lua_pop(L,1); //pop process key
       }
   }   
   lua_pop(L,1); //pop processes
   
   //then, iterate through the clusters
   lua_getfield(L, index, "count_clusters");
   luaL_checktype(L, -1, LUA_TFUNCTION);
   lua_pushvalue(L,index);
   lua_call(L,1,1);
   luaL_checktype(L, -1, LUA_TNUMBER);
   g->n_cl=lua_tointeger(L,-1);
   lua_pop(L,1); //remove number_of_clusters

   g->cl=calloc(g->n_cl,sizeof(cluster));
   lua_getfield(L,index,"clusters");
   luaL_checktype(L, -1, LUA_TFUNCTION);
   lua_pushvalue(L,index);
   lua_call(L,1,1);
   luaL_checktype(L, -1, LUA_TTABLE);
   lua_pushnil(L);
   i=0;

   while (lua_next(L, -2) != 0) {
      cluster cl=calloc(1,sizeof(struct cluster_data));
      lua_getfield (L, -2, "name"); //push the name field of cluster
      str=lua_tolstring(L, -1, &len);
      char * cname=malloc(len+1);
      memcpy(cname,str,len);
      cname[len]='\0';
      cl->name=cname;
      cl->name_len=len;
      lua_pop(L,1); //pop the name field

      lua_getfield(L,-2,"is_local");
      luaL_checktype(L, -1, LUA_TFUNCTION);
      lua_pushvalue(L,-3);
      lua_pushvalue(L,2);
      lua_pushvalue(L,3);
      lua_call(L,3,1);
      cl->local=lua_toboolean(L,-1);
      lua_pop(L,1);

      lua_getfield (L, -2, "process_addr");

      if(lua_type(L,-1)==LUA_TTABLE) {
         cl->n_processes=lua_objlen(L,-1);
         cl->processes=calloc(cl->n_processes,sizeof(process_id));
         for(j=1;j<=cl->n_processes;j++) {
            lua_rawgeti(L,-1,j); //push cluster.process[j]
            lua_pushvalue(L,-1);
            lua_gettable(L,processesid);
            int idx=lua_tointeger(L,-1);
            lua_pop(L,1);
            cl->processes[j-1]=idx;
            lua_pop(L,1);//pop cluster.process[j]
         }
      } else {
         luaL_error(L,"Cluster does not have any process");
      }
      if(create_clusters_id) {
         lua_pushvalue(L,-3);
         lua_pushinteger(L,i);
         lua_settable(L,clustersid);
         g->cl[i++]=cl;
         lua_pop(L,2); //pop cluster[key].processes && cluster key
       } else {
         lua_pushvalue(L,-3);
         lua_gettable(L,clustersid);
         int idx=lua_tointeger(L,-1);
         g->cl[idx]=cl;
         lua_pop(L,3); //pop cluster[key].processes && cluster key
       }
       
   }
   lua_pop(L,1); //pop clusters

   lua_getfield(L, index, "count_stages");
   luaL_checktype(L, -1, LUA_TFUNCTION);
   lua_pushvalue(L,index);
   lua_call(L,1,1);
   luaL_checktype(L, -1, LUA_TNUMBER);
   g->n_s=lua_tointeger(L,-1);
   lua_pop(L,1); //remove number_of_stages
   g->s=calloc(g->n_s,sizeof(stage));
   lua_getfield(L,index,"stages");
   luaL_checktype(L, -1, LUA_TFUNCTION);
   lua_pushvalue(L,index);
   lua_call(L,1,1);
   luaL_checktype(L, -1, LUA_TTABLE);
   lua_pushnil(L);
   i=0;
   //now, iterate through the stages
   while (lua_next(L, -2) != 0) {
      stage s=calloc(1,sizeof(struct stage_data));
      lua_getfield (L, -2, "serial"); //check if the serial field is present
      if(lua_isnil(L,-1)) {
         s->serial=FALSE;
      } else {
         s->serial=TRUE;
      }
      lua_pop(L,1); //pop stages[i].serial
      
      lua_getfield (L, -2, "name"); //push the name field of stage
      str=lua_tolstring(L, -1, &len); //verify if it's a string
      char * sname=malloc(len+1);
      memcpy(sname,str,len);
      sname[len]='\0';
      s->name=sname;
      s->name_len=len;
      lua_pop(L,1); //pop the name field
      
      lua_getfield (L, -2, "handler"); //push the handler field of stage
      str=lua_tolstring(L, -1, &len); //verify if it's a string
      char * handler=malloc(len+1);
      memcpy(handler,str,len);
      handler[len]='\0';
      s->handler=handler;
      s->handler_len=len;
      lua_pop(L,1); //pop the handler field
      
      lua_getfield (L, -2, "init"); //push the init field of stage
      str=lua_tolstring(L, -1, &len); //verify if its a string
      char * init=malloc(len+1);
      memcpy(init,str,len);
      init[len]='\0';
      s->init=init;
      s->init_len=len;
      lua_pop(L,1); //pop the init field
      
      lua_getfield (L, index, "get_cluster");
      lua_pushvalue(L,index);
      lua_pushvalue(L,-4);
      lua_call(L,2,1);
      lua_gettable(L,clustersid);
      s->cluster=lua_tointeger(L,-1);
      lua_pop(L,1);
      if(create_stages_id) {
         lua_pushvalue(L,-2);
         lua_pushinteger(L,i);
         lua_settable(L,stagesid);
         g->s[i++]=s;
      } else {
         lua_pushvalue(L,-2);
         lua_gettable(L,stagesid);
         int idx=lua_tointeger(L,-1);
         lua_pop(L,1);
         g->s[idx]=s;
      }
      lua_pop(L,1); //pop value
   }
   
   //then, iterate through the connectors
   lua_getfield(L, index, "count_connectors");
   luaL_checktype(L, -1, LUA_TFUNCTION);
   lua_pushvalue(L,index);
   lua_call(L,1,1);
   luaL_checktype(L, -1, LUA_TNUMBER);
   g->n_c=lua_tointeger(L,-1);
   lua_pop(L,1); //remove number_of_stages
   g->c=calloc(g->n_c,sizeof(connector));
   lua_getfield(L,index,"connectors");
   luaL_checktype(L, -1, LUA_TFUNCTION);
   lua_pushvalue(L,index);
   lua_call(L,1,1);
   luaL_checktype(L, -1, LUA_TTABLE);
   lua_pushnil(L);
   i=0;

   while (lua_next(L, -2) != 0) {
      lua_pop(L,1);
      connector c=calloc(1,sizeof(struct connector_data));
      lua_getfield (L, -1, "port"); //push the name field of connector
      str=lua_tolstring(L, -1, &len);
      char * cname=malloc(len+1);
      memcpy(cname,str,len);
      cname[len]='\0';
      c->name=cname;
      c->name_len=len;
      lua_pop(L,1); //pop the name field
      
      lua_getfield (L, -1, "method"); //push the method field of connector
      str=lua_tolstring(L, -1, &len);
      char * send=malloc(len+1);
      memcpy(send,str,len);
      send[len]='\0';
      c->send=send;
      c->send_len=len;
      lua_pop(L,1); //pop the method field
      
      lua_getfield(L, -1, "producer"); //push the producer stage
      lua_gettable(L,stagesid);
      c->p=lua_tointeger(L,-1);
      lua_pop(L,1); //pop producer id
      
      lua_getfield(L, -1, "consumer"); //push the consumer stage
      lua_gettable(L,stagesid);
      c->c=lua_tointeger(L,-1);
      lua_pop(L,1); //pop consumer id

      if(create_connectors_id) {
         lua_pushvalue(L,-1);
         lua_pushinteger(L,i);
         lua_settable(L,connectorsid);
         g->c[i++]=c;
      } else {
         lua_pushvalue(L,-1);
         lua_gettable(L,connectorsid);
         int idx=lua_tointeger(L,-1);
         lua_pop(L,1);
         g->c[idx]=c;
      }
   }

   lua_pop(L,1);//pop connectors
   
   //now, iterate again to get stages' output
   lua_pushnil(L);
   i=0;
   while (lua_next(L, -2) != 0) {
      lua_pop(L,1);

      lua_pushvalue(L,-1);
      lua_gettable(L,stagesid);
      int idx=lua_tointeger(L,-1);
      lua_pop(L,1);

      lua_getfield(L,index,"get_output");
      luaL_checktype(L, -1, LUA_TFUNCTION);
      lua_pushvalue(L,index);
      lua_pushvalue(L,-3);
      lua_call(L,2,1);
      luaL_checktype(L, -1, LUA_TTABLE);  
         
      stage s=g->s[idx];
      s->n_out=0;      
      lua_pushnil(L);
      //iterating to count the number of keys in the output of the stage
      //this is needed to allocate the right ammount of memory for outputs
      while (lua_next(L, -2) != 0) {
         if(lua_type(L, -2)==LUA_TSTRING) {
            s->n_out++;
         } else if (lua_type(L, -2)==LUA_TNUMBER) {
            s->n_out++;
         }
         lua_pop(L,1);
      }
      //now iterate again, but this time get the keys and connectors from output
      if(s->n_out>0) {
         int k=0;
         s->output=calloc(s->n_out,sizeof(key));
         lua_pushnil(L);  //first key
         while (lua_next(L, -2) != 0) { //-2 key -1 value
            if(lua_type(L, -2)==LUA_TSTRING) {
               str=lua_tolstring(L, -2, &len);
               s->output[k].type=_STRING;
               char * key=malloc(len+1);
               memcpy(key,str,len);
               key[len]='\0';
               s->output[k].key.c=key;
               lua_gettable(L,connectorsid);
               s->output[k].value=lua_tointeger(L,-1);
               lua_pop(L,1);
            } else if (lua_type(L, -2)==LUA_TNUMBER) {
               lua_Number num=lua_tonumber(L, -2);
               s->output[k].type=_NUMBER;
               s->output[k].key.n=num;
               lua_gettable(L,connectorsid);
               s->output[k].value=lua_tointeger(L,-1);
               lua_pop(L,1);
            } else {
               lua_pop(L,1);
            }
            if(s->output[k].value<0) {
               return NULL;
            }
            k++;
         }
      }
      lua_pop(L,1);      
   }
   lua_pop(L,5);

   return g;
}

/*Get a graph real_only descriptor from the lua stack*/
graph to_graph (lua_State *L, int i) {
  graph t = luaL_checkudata (L, i, GRAPH_METATABLE);
  luaL_argcheck (L, t != NULL, i, "not a read-only graph descriptor");
  return t;
}

int graph_build(lua_State * L) {
   //check if first argument is a table (with a graph representation)
   int top=lua_gettop(L);
   luaL_checktype(L,1, LUA_TTABLE);
   lua_getfield(L,1,"is_graph");
   if(lua_type(L,-1)!=LUA_TFUNCTION) luaL_error(L,"Parameter #1 must be a graph");
   lua_pop(L,1);
   
   //build graph representation   
   graph g=(graph)lua_newuserdata (L,sizeof(struct graph_data));
   //set the graph metatable for the userdata
   luaL_getmetatable (L, GRAPH_METATABLE);
   lua_setmetatable(L,-2);
   
   g=build_graph_representation(L,1,g);
   if(g)
      return 1;
   lua_settop(L,top);
   luaL_error(L,"Error building graph representation");
   return 0;
}

#define NULL_SAFE_FREE(p) if((p)) free((void*)(p))

/* destoy a graph representation from the memory */
int graph_destroy(lua_State* L) {
   graph g=to_graph(L,1);
   //printf("Collecting graph '%s'\n",g->name);
   int i;
   for(i=0;i<g->n_s;i++) {
      NULL_SAFE_FREE(g->s[i]->name);
      NULL_SAFE_FREE(g->s[i]->handler);
      NULL_SAFE_FREE(g->s[i]->init);
      NULL_SAFE_FREE(g->s[i]->output);
      NULL_SAFE_FREE(g->s[i]);
   }
   NULL_SAFE_FREE(g->s);
   
   for(i=0;i<g->n_c;i++) {
      NULL_SAFE_FREE(g->c[i]->name);
      NULL_SAFE_FREE(g->c[i]->send);
      NULL_SAFE_FREE(g->c[i]);
   }
   NULL_SAFE_FREE(g->c);
   
   for(i=0;i<g->n_cl;i++) {
      NULL_SAFE_FREE(g->cl[i]->name);
      NULL_SAFE_FREE(g->cl[i]->processes);
      NULL_SAFE_FREE(g->cl[i]);
   }
   NULL_SAFE_FREE(g->cl);
   
   for(i=0;i<g->n_d;i++) {
      NULL_SAFE_FREE(g->d[i]->host);
      NULL_SAFE_FREE(g->d[i]);
   }   
   
   NULL_SAFE_FREE(g->d);
   return 0;
}

/* Dump a graph representation for debug purposes*/
int graph_dump(lua_State * L) {
   graph g=to_graph(L,1);
   int i,j;
   fprintf(stderr,"==== Dumping graph: '%s' ====\n",g->name);
   fprintf(stderr,"==== Clusters (%d) ====\n",(int)g->n_cl);
   for(i=0;i<g->n_cl;i++) {
      fprintf(stderr,"\tCluster: id=%d name='%s' processes='%d' local='%d'\n",i,g->cl[i]->name,(int)g->cl[i]->n_processes,g->cl[i]->local);
      for(j=0;j<g->cl[i]->n_processes;j++) {
         fprintf(stderr,"\t\tProcess #%d: '%s:%d'\n",j+1,g->d[g->cl[i]->processes[j]]->host,g->d[g->cl[i]->processes[j]]->port);
      }
   }
   fprintf(stderr,"==== Stages (%d) ====\n",(int)g->n_s);
   for(i=0;i<g->n_s;i++) {
      fprintf(stderr,"\tStage: id='%d' name='%s' serial='%d' cluster='%s'\n",i,g->s[i]->name,g->s[i]->serial,g->cl[g->s[i]->cluster]->name);
//     fprintf(stderr,"\tStage handler: %s\n",g->s[i]->handler);
       for(j=0;j<g->s[i]->n_out;j++) {
         switch(g->s[i]->output[j].type) {
            case _STRING:
               fprintf(stderr,"\t\tOutput: key='%s' connector='%s' id='%d'\n",g->s[i]->output[j].key.c,g->c[g->s[i]->output[j].value]->name,(int)g->s[i]->output[j].value);
               break;
            case _NUMBER:               
               fprintf(stderr,"\t\tOutput: key='%f' connector='%s' id='%d'\n",g->s[i]->output[j].key.n,g->c[g->s[i]->output[j].value]->name,(int)g->s[i]->output[j].value);
          }
      }

   }
     fprintf(stderr,"==== Connectors (%d) ====\n",(int)g->n_c);
   for(i=0;i<g->n_c;i++) {
      fprintf(stderr,"\tConnector: id='%d' name='%s' prod='%s' cons='%s'\n",i,g->c[i]->name,g->s[g->c[i]->p]->name,g->s[g->c[i]->c]->name);
   }

   fprintf(stderr,"========\n");
   return 0;
}

/*create a unique graph metatable*/
int graph_createmetatable (lua_State *L) {
	/* Create graph metatable */
	if (!luaL_newmetatable (L, GRAPH_METATABLE)) {
		return 0;
	}
   lua_pushliteral(L,"dump");
   lua_pushcfunction(L,graph_dump);
   lua_rawset(L,-3);
   
	/* define metamethods */
	lua_pushliteral (L, "__index");
	lua_pushvalue (L, -2);
	lua_settable (L, -3);
	
	lua_pushliteral (L, "__gc");
	lua_pushcfunction (L, graph_destroy);
	lua_settable (L, -3);

	lua_pushliteral (L, "__metatable");
	lua_pushliteral (L, "You're not allowed to get the metatable of a Thread");
	lua_settable (L, -3);
	
	lua_pop(L,1);
	
	return 0;
}
