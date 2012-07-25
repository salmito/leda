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

/* get a connector representation from the 'id' defined on graph 'g' */
connector graph_get_connector(graph g,connector_id id) {
   if(!g) return NULL;
   if(id > g->n_c) return NULL;
   return g->c[id];
}

/* get a stage representation from the 'id' defined on graph 'g' */
stage graph_get_stage(graph g,stage_id id) {
   if(!g) return NULL;
   if(id > g->n_s) return NULL;
   return g->s[id];
}

/* lookup the correspondent connector_id from a connector unique id
 */
connector_id get_connector_id_from_ptr(graph g, void * id) {
   int i;
   if(!g) return -1; //Graph is null

   for(i=0;i<g->n_c;i++) {
      if(g->c[i]->unique_id==id) 
         return i;
   }
   
   return -2; //connector not found
}

/* lookup the correspondent stage_id from a stage unique id
 */
stage_id get_stage_id_from_ptr(graph g, void * id) {
   int i;
   if(!g) return -1; //Graph is null

   for(i=0;i<g->n_s;i++) {
      if(g->s[i]->unique_id==id) 
         return i;
   }
   
   return -2; //stage not found
}


/* Build a graph internal representation for the graph defined in
 * in the stack on 'index'
 */
graph build_graph_representation(lua_State *L, int index) {
   int i,n;
   //argument must be a table with a graph definition
   luaL_checktype(L,index, LUA_TTABLE);

   struct graph_data * g=calloc(1,sizeof(struct graph_data));
   
   char const * str;
   size_t len;
   
   lua_getfield (L, index, "name"); //push the name field of stage
   str=lua_tolstring(L, -1, &len); //verify if its a string
   char * gname=malloc(len+1);
   memcpy(gname,str,len);
   gname[len]='\0';
   g->name=gname;
   lua_pop(L,1); //pop the name field

   //first, iterate thgough the connectors field
   lua_pushstring(L,"connectors");
   lua_rawget(L, index);
   
   luaL_checktype(L,-1, LUA_TTABLE);
   n=lua_objlen(L,-1); //get size of 'connectors' field
   

   struct connector_data ** connectors=calloc(n,sizeof(struct connector_data *));
      
   for(i=1;i<=n;i++) {
      struct connector_data * c=calloc(1,sizeof(struct connector_data));
      lua_rawgeti(L,-1,i); //push g.connectors[i]
      c->unique_id=(void *)lua_topointer(L,-1);
      
      lua_getfield (L, -1, "name"); //push the name field of connector
      str=lua_tolstring(L, -1, &len);
      char * cname=malloc(len+1);
      memcpy(cname,str,len);
      cname[len]='\0';
      c->name=cname;
      lua_pop(L,1); //pop the name field
      
      lua_getfield (L, -1, "sendf"); //push the send field of connector
      str=lua_tolstring(L, -1, &len);
      char * send=malloc(len+1);
      memcpy(send,str,len);
      send[len]='\0';
      c->send=send;
      c->send_len=len;
      lua_pop(L,1); //pop the send field
   
      lua_pop(L,1); //pop g.connectors[i]
      connectors[i-1]=c;
   }
   lua_pop(L,1); //pop the connectors field
   
   g->c=(connector *)connectors;
   g->n_c=n;

        
   //then, iterate thgough the stages field
   lua_pushstring(L,"stages");
   lua_rawget(L, index);
   luaL_checktype(L,-1, LUA_TTABLE);
   n=lua_objlen(L,-1); //get size of 'stages' field
   
   struct stage_data ** stages=calloc(n,sizeof(struct stage_data *));
      
   for(i=1;i<=n;i++) {
      struct stage_data * s=calloc(1,sizeof(struct stage_data));

      lua_rawgeti(L,-1,i); //push stages[i]
      s->unique_id=(void *)lua_topointer(L,-1);

      lua_getfield (L, -1, "name"); //push the name field of stage
      str=lua_tolstring(L, -1, &len); //verify if its a string
      char * name=malloc(len+1);
      memcpy(name,str,len);
      name[len]='\0';
      s->name=name;
      lua_pop(L,1); //pop the name field
      
      lua_getfield (L, -1, "handler"); //push the handler field of stage
      str=lua_tolstring(L, -1, &len); //verify if its a string
      char * handler=malloc(len+1);
      memcpy(handler,str,len);
      handler[len]='\0';
      s->handler=handler;
      s->handler_len=len;
      lua_pop(L,1); //pop the handler field
      
      lua_getfield (L, -1, "init"); //push the init field of stage
      str=lua_tolstring(L, -1, &len); //verify if its a string
      char * init=malloc(len+1);
      memcpy(init,str,len);
      init[len]='\0';
      s->init=init;
      s->init_len=len;
      lua_pop(L,1); //pop the init field

      //iterate through the output field
      
      lua_pushstring(L,"output");
      lua_rawget(L, -2); //push stage[i].output
      
      luaL_checktype(L,-1, LUA_TTABLE); 
      lua_pushnil(L);  //first key
      s->n_out=0;      
      //iterating to count the number of keys in the output of the stage
      //it is needed to allocate memory for outputs
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
               s->output[k].value=get_connector_id_from_ptr(g, (void *)lua_topointer(L,-1));
            } else if (lua_type(L, -2)==LUA_TNUMBER) {
               lua_Number num=lua_tonumber(L, -2);
               s->output[k].type=_NUMBER;
               s->output[k].key.n=num;
               s->output[k].value=get_connector_id_from_ptr(g, (void *) lua_topointer(L,-1));
            }

            if(s->output[k].value<0) {
               graph_destroy(g);
               return NULL;
            }

            lua_pop(L,1);
            k++;
         }
      }
  
      lua_pop(L,1); //pop stage[i].output
   

      lua_pop(L,1); //pop stages[i]      
      stages[i-1]=s;
   }
   lua_pop(L,1); //pop the g.stages field
   g->s=(stage *)stages;
   g->n_s=n;

   //finally, allocate the producers and 
   //consumers memory references for connectors
   lua_pushstring(L,"connectors");
   lua_rawget(L, index);

   luaL_checktype(L,-1, LUA_TTABLE);
   n=lua_objlen(L,-1); //get size of 'connectors' field
   
    for(i=1;i<=n;i++) {
      int n2,j;
      lua_rawgeti(L,-1,i); //push connectors[i]
      
      lua_pushstring(L,"producers");
      lua_rawget(L, -2); //push connetors[i].producers
      luaL_checktype(L, -1, LUA_TTABLE);
      n2=lua_objlen(L,-1); //Size of producers
      g->c[i-1]->n_p=n2;  
      g->c[i-1]->p=calloc(n2,sizeof(stage_id));
      
      for(j=1;j<=n2;j++) {
         lua_rawgeti(L,-1,j); //push producers[j]
         g->c[i-1]->p[j-1]=get_stage_id_from_ptr(g, (void *)lua_topointer(L,-1));
         lua_pop(L,1); //pop producers[i]
      }
      lua_pop(L,1); //pop c.producers
      
      lua_getfield (L, -1, "consumers"); //push c.consumers
      luaL_checktype(L, -1, LUA_TTABLE);
      n2=lua_objlen(L,-1); //Size of consumers
      g->c[i-1]->n_c=n2;
      g->c[i-1]->c = calloc(n2,sizeof(stage_id));
      
      for(j=1;j<=n2;j++) {
         lua_rawgeti(L,-1,j); //push consumers[j]
         g->c[i-1]->c[j-1]=get_stage_id_from_ptr(g, (void *)lua_topointer(L,-1));
         lua_pop(L,1); //pop consumers[i]
      }
      
      lua_pop(L,1); //pop c.consumers
      lua_pop(L,1);
   }
   lua_pop(L,1); //pop g.connectors


   return g;
}

#define NULL_SAFE_FREE(p) if(p) free((void*)p)

/* destoy a graph representation from the memory */
void graph_destroy(graph g) {
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
      NULL_SAFE_FREE(g->c[i]->p);
      NULL_SAFE_FREE(g->c[i]->c);
      NULL_SAFE_FREE(g->c[i]->send);
      NULL_SAFE_FREE(g->c[i]);
   }
   NULL_SAFE_FREE(g->c);
}

/* Dump a graph representation for debug purposes*/
void graph_dump(graph g) {
   #ifndef DEBUG
      return;
   #endif
   int i,j;
   _DEBUG("==== Dumping graph: '%s' ====\n",g->name);
   _DEBUG("\t==== Stages (%d) ====\n",(int)g->n_s);
   for(i=0;i<g->n_s;i++) {
      _DEBUG("\tStage: id='%d' unique_id='%p' name='%s'\n",i,g->s[i]->unique_id,g->s[i]->name);
     _DEBUG("\t\tHandler function: %s\n",g->s[i]->handler);
     _DEBUG("\t\tInit function: %s\n",g->s[i]->init);
      for(j=0;j<g->s[i]->n_out;j++) {
         switch(g->s[i]->output[j].type) {
            case _STRING:
               _DEBUG("\t\tOutput: key='%s' connector='%s'\n",g->s[i]->output[j].key.c,g->c[g->s[i]->output[j].value]->name);
               break;
            case _NUMBER:               
               _DEBUG("\t\tOutput: key='%f' connector='%s'\n",g->s[i]->output[j].key.n,g->c[g->s[i]->output[j].value]->name);
          }
      }
   }
   _DEBUG("\t==== Connectors (%d) ====\n",(int)g->n_c);
   for(i=0;i<g->n_c;i++) {
      _DEBUG("\tConnector: id='%d' unique_id='%p' name='%s' prods='%d' cons='%d'\n",i,g->c[i]->unique_id,g->c[i]->name,(int)g->c[i]->n_p,(int)g->c[i]->n_c);
      _DEBUG("\t\tSend function: %s\n",g->c[i]->send);
      _DEBUG("\t\tProducers: ");
      for(j=0;j<g->c[i]->n_p;j++) {
         _DEBUG("%d (%s) ",(int)g->c[i]->p[j],g->s[g->c[i]->p[j]]->name);
      }
      _DEBUG("\n");
      _DEBUG("\t\tConsumers: ");
      for(j=0;j<g->c[i]->n_c;j++) {
         _DEBUG("%d (%s) ",(int)g->c[i]->c[j],g->s[g->c[i]->c[j]]->name);
      }
      _DEBUG("\n");

   }
   _DEBUG("========\n");
}


