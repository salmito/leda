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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <lualib.h>

#include "event.h"
#include "instance.h"
#include "thread.h"
#include "extra/lmarshal.h"

#include <sys/epoll.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAXN 1024

struct _element {
   int type; //Type of element
   union  {
      lua_Number n;
      char * str;
    	bool_t b;
   	void * ptr;
//   	element table; TODO
   } data;
   size_t len; //Size of data (if type is 'string' it holds the size of string)
};

static THREAD_T event_thread;

/* Free the payload vector of an event.
 * 
 * If an element is a string, it will be freed as well
 */
void destroy_event(event e) {
   size_t i;
   for(i=0;i<e->n;i++) {
      switch(e->payload[i].type) {
         case LUA_TSTRING:
         case LUA_TFUNCTION:
         case LUA_TTABLE:
         case LUA_TUSERDATA:
         if(e->payload[i].data.str) 
            free(e->payload[i].data.str);
         }
    }
   free(e->payload);
   free(e);
}

/* Dump an event for debug purposes */
void dump_event(lua_State *L, event e) {
   size_t i;
   for(i=0;i<e->n;i++) {
      _DEBUG("Event: DUMP element->type='%s' len='%u' ",
      lua_typename(L,e->payload[i].type),
      (uint_t)e->payload[i].len);
      
      switch (e->payload[i].type) {
      case LUA_TBOOLEAN:
         _DEBUG("value='%s' ",(e->payload[i].data.b?"TRUE":"FALSE"));
         break;
      case LUA_TNUMBER:
         _DEBUG("value='%f' ",e->payload[i].data.n);
         break;
      case LUA_TSTRING:
         _DEBUG("value='%s' length='%d'",e->payload[i].data.str,(int)e->payload[i].len);
         break;
      case LUA_TLIGHTUSERDATA:
         _DEBUG("value='%p' ",e->payload[i].data.ptr);
         break;
      }
     _DEBUG("\n");
   }
}

/* restore an event to a Lua stack.
 * The event payload is pushed back to the top of the stack
 */
int restore_event_to_lua_state(lua_State * L, event *e_t) {
   size_t i;
   event e=*e_t;
   
   for(i=0;i<e->n;i++) {
      switch ( e->payload[i].type ) {
         case LUA_TNIL:
            lua_pushnil(L);
            break;
         case LUA_TBOOLEAN:
            lua_pushboolean(L, e->payload[i].data.b);
            break;
         case LUA_TNUMBER:
            lua_pushnumber(L, e->payload[i].data.n);
            break;
         case LUA_TSTRING:
            lua_pushlstring(L, e->payload[i].data.str,e->payload[i].len);
            break;
         case LUA_TLIGHTUSERDATA:
            lua_pushlightuserdata(L, e->payload[i].data.ptr);
            break;
         case LUA_TFUNCTION:
         case LUA_TTABLE:
         case LUA_TUSERDATA:
            lua_pushcfunction(L,mar_decode);
            lua_pushlstring(L, e->payload[i].data.str,e->payload[i].len);
            lua_call(L,1,1);
            break;
      }
   }
   int ret=e->n;
   destroy_event(e);
   *e_t=NULL;
   return ret;
}


/*
 * Copies values directly from lua_State src to lua_State dst. 
 * (code taken from luarings, with small adjustments)
 */
void copy_values_directly 
     (lua_State *dst, lua_State *src, int from, int args) {
  int i;
  if(args==0) return; //nothing to copy
 lua_checkstack(dst, from+args);
  
  for (i=from; i < from+args; i++) {
    switch (lua_type (src, i)) {
      case LUA_TNUMBER:
        lua_pushnumber (dst, lua_tonumber (src, i));
        break;
      case LUA_TBOOLEAN:
        lua_pushboolean (dst, lua_toboolean (src, i));
        break;
      case LUA_TSTRING: {
        const char *string = lua_tostring (src, i);
        size_t length = lua_objlen (src, i);
        lua_pushlstring (dst, string, length);
        break;
      }
      case LUA_TLIGHTUSERDATA: {
        lua_pushlightuserdata (dst, lua_touserdata (src, i));
        break;
      }
      case LUA_TNIL:
        lua_pushnil (dst);
        break;
      case LUA_TFUNCTION:
      case LUA_TTABLE:
      case LUA_TUSERDATA: {
          lua_pushcfunction(src,mar_encode);
          lua_pushvalue(src,i);
          lua_call(src,1,1);
          size_t len; const char *s = lua_tolstring( src, -1, &len );
          lua_pushcfunction(dst,mar_decode);
          lua_pushlstring (dst, s, len);
          lua_call(dst,1,1);
          lua_pop(src,1);
         } break;              
      default:
         lua_pushfstring(src,"Value type '%s' not supported",
         lua_typename(src,lua_type(src,i)));
         lua_error(src);
        break;
    }
  }
}

/*
** Copies an event element from the Lua stack
*/
bool_t copy_event_element(lua_State *L, size_t i, element e) {
   switch ( lua_type(L,i) ) {
      case LUA_TNIL:
         e->type=LUA_TNIL;
         e->len=0;
         break;
      case LUA_TBOOLEAN:
         e->type=LUA_TBOOLEAN;
         e->len=sizeof(bool_t);
         e->data.b=lua_toboolean(L, i);
         break;
      case LUA_TNUMBER:
         e->type=LUA_TNUMBER;
         e->data.n=lua_tonumber(L, i);
         e->len=sizeof(lua_Number);
         break;
       case LUA_TSTRING: {
         size_t len; const char *s = lua_tolstring( L, i, &len );
         e->type=LUA_TSTRING;
         /* We have to allocate a new memory chunk for the string
         because the garbage collector may destroy the string 's'
         before the event consumption.
         It will be deallocated in the event_destroy() function */
         e->data.str=malloc(len+1); //space for \0
         e->len=len;
         memcpy(e->data.str,s,len);
         e->data.str[len]='\0';
         } break;
       case LUA_TLIGHTUSERDATA:
         e->type=LUA_TLIGHTUSERDATA;
         e->data.ptr=lua_touserdata(L, i);
         e->len=sizeof(void *);
         break;
       case LUA_TFUNCTION:
       case LUA_TTABLE:
       case LUA_TUSERDATA: {
          e->type=lua_type(L,i);
          lua_pushcfunction(L,mar_encode);
          lua_pushvalue(L,i);
          lua_call(L,1,1);
          size_t len; const char *s = lua_tolstring( L, -1, &len );
          e->data.str=malloc(len);
          e->len=len;
          memcpy(e->data.str,s,len);
          lua_pop(L,1);
          } break;
       default:
         return FALSE;
   }
   return TRUE;
}

/* extract a event from a lua stack of length 'args' from the stack 
 * index 'from' (inclusive).
 */
event extract_event_from_lua_state(lua_State *L, int from, int args) {
   int i,j=0;
   bool_t ok=TRUE;

	event e=malloc(sizeof(struct event_data));
   e->n=args; //number of elements of the payload to be copyed
	
	e->payload=calloc(e->n,sizeof(struct _element));
	
   for(i = from; i < from+args; ++ i) {
      if(!(ok=copy_event_element(L, i, &e->payload[j++])))
         break;
	}
	
	if(!ok) {
      //error occured at i-th position
   	destroy_event(e);
      lua_pushfstring(L,"Value type '%s' not supported",
      lua_typename(L,lua_type(L,i)));
      lua_error(L);
  	}
  	return e; //copyed ok
}

#define EVENT_TYPE 28
#define INIT_TYPE 27

queue * sockets;
atomic * cur_process;
lua_State * dummy;

/* send an event through the network */
int send_event(lua_State *L) {
   stage_id s_id=lua_tointeger(L,1);
   luaL_checktype(L,2,LUA_TTABLE);
   
   lua_pushcfunction(L,mar_encode);
   lua_pushvalue(L,2);
   lua_call(L,1,1); //propagate error
   luaL_checktype(L,-1,LUA_TSTRING);
   size_t len; const char *payload=lua_tolstring(L,-1,&len); 

   int next_d=STORE(cur_process[STAGE(s_id)->cluster],
   (READ(cur_process[STAGE(s_id)->cluster])+1)%CLUSTER(STAGE(s_id)->cluster)->n_processes);
   
   process_id dst_id=CLUSTER(STAGE(s_id)->cluster)->processes[next_d];
   int sockfd;
   if(!TRY_POP(sockets[dst_id],sockfd)) {
      _DEBUG("Process: Connecting to process '%s:%d'\n",PROCESS(dst_id)->host,PROCESS(dst_id)->port);
      struct sockaddr_in adr_inet;
      int len_inet;
      adr_inet.sin_family = AF_INET;  
      adr_inet.sin_port = htons(PROCESS(dst_id)->port); 
      if (!inet_aton(PROCESS(dst_id)->host,&adr_inet.sin_addr) ) {
        lua_pop(L,1);
        lua_pushboolean(L,FALSE);
        lua_pushfstring(L,"Bad address '%s'",PROCESS(dst_id)->host);
        return 2;
      }
      len_inet = sizeof adr_inet; 
      if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        lua_pop(L,1);
        lua_pushboolean(L,FALSE);
        lua_pushliteral(L,"Could not create socket");
        return 2;
      }
      if(connect(sockfd, (struct sockaddr *)&adr_inet, sizeof(adr_inet)) < 0) {
        lua_pop(L,1);
        lua_pushboolean(L,FALSE);
        lua_pushfstring(L,"Could not establish connection with process '%s:%d'",PROCESS(dst_id)->host,PROCESS(dst_id)->port);
        return 2;
      }
      _DEBUG("Process: Connected to process '%s:%d'\n",PROCESS(dst_id)->host,PROCESS(dst_id)->port);
   }
   char c=EVENT_TYPE;
   int size=write(sockfd,&c,1);
   if(size!=1) {
      lua_pop(L,1);
      close(sockfd);
      lua_pushboolean(L,FALSE);
      lua_pushfstring(L,"Error sending event to process '%s:%d': %s",PROCESS(dst_id)->host,PROCESS(dst_id)->port,strerror(errno));
   }

   size=write(sockfd,&s_id,sizeof(stage_id));
   if(size!=sizeof(stage_id)) {
      lua_pop(L,1);
      close(sockfd);
      lua_pushboolean(L,FALSE);
      lua_pushfstring(L,"Error sending event to process '%s:%d': %s",PROCESS(dst_id)->host,PROCESS(dst_id)->port,strerror(errno));
   }
   size=write(sockfd,&len,sizeof(size_t));
   if(size!=sizeof(size_t)) {
      lua_pop(L,1);
      close(sockfd);   
      lua_pushboolean(L,FALSE);
      lua_pushfstring(L,"Error sending event to process '%s:%d': %s",PROCESS(dst_id)->host,PROCESS(dst_id)->port,strerror(errno));
   }
   
   int writed=0;
   while(writed<len) {
      size=write(sockfd,payload+writed,len-writed);
      if(size<0) {
         lua_pop(L,1);
         close(sockfd);      
         lua_pushboolean(L,FALSE);
         lua_pushfstring(L,"Error sending event to process '%s:%d': %s",PROCESS(dst_id)->host,PROCESS(dst_id)->port,strerror(errno));      
         return 2;
      } else if(size==0) {
         lua_pop(L,1);
         close(sockfd);
         lua_pushboolean(L,FALSE);
         lua_pushfstring(L,"Error sending event to process '%s:%d': Process closed connection",PROCESS(dst_id)->host,PROCESS(dst_id)->port);
         return 2;
      }
      writed+=size;
   }
   char res=0;
  
   size=read(sockfd,&res,sizeof(char));
   if(size!=sizeof(char)) {
      lua_pop(L,1);
      close(sockfd);
      lua_pushboolean(L,FALSE);
      lua_pushfstring(L,"Error sending event to process '%s:%d': %s",PROCESS(dst_id)->host,PROCESS(dst_id)->port,strerror(errno));
      return 2;
   }
   if(res==1) {
      lua_pop(L,1);
      if(!TRY_PUSH(sockets[dst_id],sockfd)) 
         close(sockfd);
      lua_pushboolean(L,TRUE);
      _DEBUG("Process: Sent remote event\n");
      return 1;
   } 
   char buf[2048];
   size=read(sockfd,buf,2048);
   if(size<=0) {
      lua_pop(L,1);
      close(sockfd);
      lua_pushboolean(L,FALSE);
      lua_pushfstring(L,"Error sending event to process '%s:%d': %*s",PROCESS(dst_id)->host,PROCESS(dst_id)->port,buf,strerror(errno));
      return 2;
   }
   lua_pop(L,1);
   close(sockfd);
   lua_pushboolean(L,FALSE);
   lua_pushfstring(L,"Error sending event to process '%s:%d': %*s",PROCESS(dst_id)->host,PROCESS(dst_id)->port,buf,size);
   return 2;
}

/* read an event from a socket ready for reading */
int read_event(int fd) {
   char type=0;
   int size=read(fd,&type,1);
   if(size!=1) return 1;
   if(type==INIT_TYPE) {
      size=write(fd,"Deamon has already started\n",27);
      _DEBUG("Process: Error: Deamon has already started\n");
      close(fd);
   } else if(type!=EVENT_TYPE) return 1;

   stage_id id;
   size=read(fd,&id,sizeof(stage_id));
   if(size!=sizeof(stage_id)) return 1;

   size_t len=0;
   size=read(fd,&len,sizeof(size_t));
   if(size!=sizeof(size_t)) return 1;

   int readed=0;
   if(len<=0) return 1;
   char * buf=malloc(len);
   if(!buf) return 1;

   while(readed<len) {
      size=read(fd,buf+readed,len-readed);
      if(size<=0) {
         free(buf);
         return 1;
      }
      readed+=size;
   }
   if(readed!=len) {
      free(buf);
      return 1;
   }

   lua_pushcfunction(dummy,emmit);
   lua_pushinteger(dummy,id);
   lua_pushinteger(dummy,-1); //connector id not known
   int begin=lua_gettop(dummy);
   lua_getglobal(dummy,"unpack"); //Push unpack function
   lua_pushcfunction(dummy,mar_decode);
   lua_pushlstring(dummy,buf,len);
   free(buf);
   lua_call(dummy,1,1); //decode event
   lua_call(dummy,1,LUA_MULTRET); //Unpack event
   int args=lua_gettop(dummy)-begin;

   lua_call(dummy,args+2,2);
   
   if(lua_toboolean(dummy,-2)==TRUE) {
      char res=TRUE;
      size=write(fd,&res,sizeof(char));
      if(size!=sizeof(char)) return 1;
   } else {
      char res=FALSE;
      size=write(fd,&res,sizeof(char));
      if(size!=sizeof(char)) return 1;
      const char * b=lua_tolstring(dummy,-1,&len);
      size=write(fd,b,len);
      return 1;
   }
   return 0;
}

int leda_gettime(lua_State * L) {
   lua_pushnumber(L,now_secs());
   return 1;
}

struct event_epoll_data {
   int fd;
   instance i;
   bool_t timer;
};

int epfd;

void event_wait_io(instance i) {
   struct epoll_event event; 
   //FIXME CAREFULL, ERRORS ARE UNPROTECTED. IT DOES A LONGJUMP SO
   //THE THREAD WILL BE OUT OF CONTEXT IF SOMETHING HAPPENS
   
   int fd=-1;
   if (lua_type(i->L,1)==LUA_TNUMBER) {
      fd=lua_tointeger(i->L,1);
   } else {
       lua_settop(i->L,0);
       //Get the  main coroutine of the instance's handler
       lua_getglobal(i->L, "handler");
       //Put it on the bottom of the instance's stack
       lua_pushnil(i->L);
       lua_pushliteral(i->L,"Invalid argument");
       //Set the previous number of arguments
       i->args=2;
       push_ready_queue(i);
       return;
   }
   int mode=-1;
   if (lua_type(i->L,2)==LUA_TNUMBER) {
      mode=lua_tointeger(i->L,2);
   } else {
      lua_settop(i->L,0);
      //Get the  main coroutine of the instance's handler
      lua_getglobal(i->L, "handler");
      //Put it on the bottom of the instance's stack
      lua_pushnil(i->L);
      lua_pushliteral(i->L,"Invalid argument");
      //Set the previous number of arguments
      i->args=2;
      push_ready_queue(i);
      return;
   }
   
   int err;

   struct event_epoll_data * ed=malloc(sizeof(struct event_epoll_data));
   
   //FIXME DITTO
   if(!ed) {
       lua_settop(i->L,0);
       //Get the  main coroutine of the instance's handler
       lua_getglobal(i->L, "handler");
       //Put it on the bottom of the instance's stack
       lua_pushnil(i->L);
       lua_pushliteral(i->L,"Malloc error");
       //Set the previous number of arguments
       i->args=2;
       push_ready_queue(i);
       return;
   }

   ed->fd=fd;
   ed->i=i;
   ed->timer=0;

   event.data.ptr = ed; /* return the fd to us later */

   if(mode==1) 
      event.events = EPOLLIN /*read*/;
   else if(mode==2)
         event.events = EPOLLOUT /*write*/;
   else if(mode==3)
         event.events = EPOLLIN /*read*/| EPOLLOUT /*write*/;
   else {
       lua_settop(i->L,0);
       //Get the  main coroutine of the instance's handler
       lua_getglobal(i->L, "handler");
       //Put it on the bottom of the instance's stack
       lua_pushnil(i->L);
       lua_pushliteral(i->L,"Invalid argument");
       //Set the previous number of arguments
       i->args=2;
       push_ready_queue(i);
       return;
   }
   

   if((err = epoll_ctl (epfd, EPOLL_CTL_ADD, ed->fd, &event))) {
       free(ed);
       close(ed->fd);
       lua_settop(i->L,0);
       //Get the  main coroutine of the instance's handler
       lua_getglobal(i->L, "handler");
       //Put it on the bottom of the instance's stack
       lua_pushnil(i->L);
       lua_pushfstring(i->L,"Epoll ERROR: %s",strerror(errno));
       //Set the previous number of arguments
       i->args=2;
       push_ready_queue(i);
       return;
   }
}

#include <sys/timerfd.h>
 
void event_sleep(instance i) {
   struct epoll_event event; 
   //FIXME CAREFULL, THIS ERROR IS UNPROTECTED. IT DOES A LONGJUMP SO
   //THE THREAD WILL BE OUT OF CONTEXT IF IT HAPPENS
   double time=now_secs()+lua_tonumber(i->L,1);
   if(time<=0.0) {
       lua_settop(i->L,0);
       //Get the  main coroutine of the instance's handler
       lua_getglobal(i->L, "handler");
       //Put it on the bottom of the instance's stack
       lua_pushnil(i->L);
       lua_pushliteral(i->L,"Invalid timer");
       //Set the previous number of arguments
       i->args=2;
       push_ready_queue(i);
       return;
   }
   time_t secs=time;
   long nsecs=(time-secs)*1000000000L;
   
   int err;

   struct event_epoll_data * ed=malloc(sizeof(struct event_epoll_data));
   
   if(!ed) {
       lua_settop(i->L,0);
       //Get the  main coroutine of the instance's handler
       lua_getglobal(i->L, "handler");
       //Put it on the bottom of the instance's stack
       lua_pushnil(i->L);
       lua_pushliteral(i->L,"Malloc error");
       //Set the previous number of arguments
       i->args=2;
       push_ready_queue(i);
       return;
   }
  
   ed->fd=timerfd_create(CLOCK_REALTIME, 0);
   if(ed->fd < 0) {
       free(ed);
       lua_settop(i->L,0);
       //Get the  main coroutine of the instance's handler
       lua_getglobal(i->L, "handler");
       //Put it on the bottom of the instance's stack
       lua_pushnil(i->L);
       lua_pushfstring(i->L,"Error creating timer: %s",strerror(errno));
       //Set the previous number of arguments
       i->args=2;
       push_ready_queue(i);
       return;
   }
   struct itimerspec t={{0,0L},{0,0L}};
   t.it_value.tv_sec=secs;
   t.it_value.tv_nsec=nsecs;
   
   if(timerfd_settime(ed->fd, TFD_TIMER_ABSTIME, &t, NULL) < 0) {
       free(ed);
       close(ed->fd);
       lua_settop(i->L,0);
       //Get the  main coroutine of the instance's handler
       lua_getglobal(i->L, "handler");
       //Put it on the bottom of the instance's stack
       lua_pushnil(i->L);
       lua_pushfstring(i->L,"Error setting timer: %s",strerror(errno));
       //Set the previous number of arguments
       i->args=2;
       push_ready_queue(i);
       return;
   }
   ed->i=i;
   ed->timer=1;

   event.data.ptr = ed; /* return the fd to us later */

   event.events = EPOLLIN;   

   if((err = epoll_ctl (epfd, EPOLL_CTL_ADD, ed->fd, &event))) {
       free(ed);
       close(ed->fd);
       lua_settop(i->L,0);
       //Get the  main coroutine of the instance's handler
       lua_getglobal(i->L, "handler");
       //Put it on the bottom of the instance's stack
       lua_pushnil(i->L);
       lua_pushfstring(i->L,"Epoll error: %s",strerror(errno));
       //Set the previous number of arguments
       i->args=2;
       push_ready_queue(i);
   }
}


static THREAD_RETURN_T THREAD_CALLCONV event_main(void *t_val) {
   int process_fd=*(int*)t_val;
   free(t_val);
   struct event_epoll_data * ed=malloc(sizeof(struct event_epoll_data));
   epfd = epoll_create (1024);
   if(epfd==-1) {
      perror("Leda PANIC: Error creating epfd");
   }
   {
      struct epoll_event event;
      ed->i=NULL;
      ed->fd=process_fd;
      ed->timer=0;
      event.data.ptr = ed;
      event.events = EPOLLIN;
   
      if(epoll_ctl (epfd, EPOLL_CTL_ADD, process_fd, &event)) {
         perror("Leda PANIC: Error on process_fd");
      }
   }
   
   _DEBUG("Process: Waiting for incomming events\n")
   while(TRUE) {
      struct epoll_event events[128];
      int nr_events, i;
   
      nr_events = epoll_wait (epfd, events, 128, -1);
      if (nr_events < 0) { //epoll error
         perror("Epoll error");
      }
      for (i = 0; i < nr_events; i++) {
         if(events[i].events & EPOLLIN) { //fd available for read
            ed=events[i].data.ptr;
            if(ed->fd==process_fd) { //new connection for process
               _DEBUG("Process: Incomming new connection\n");
               socklen_t addrlen=sizeof(struct sockaddr_in);
               struct sockaddr_in address;
               int client_fd = accept(process_fd, (struct sockaddr *)&address, &addrlen);
               if (client_fd<0) {
                  _DEBUG("Error accepting connection");
                  continue;
               }
               
               struct epoll_event event;
               
               struct event_epoll_data * ned=malloc(sizeof(struct event_epoll_data));
               ned->fd=client_fd;
               ned->i=NULL;
               ned->timer=0;
               event.data.ptr = ned;
               event.events = EPOLLIN;
   
               if(epoll_ctl (epfd, EPOLL_CTL_ADD, client_fd, &event)) {
                  perror("Epoll error");
               }
            } else if(ed->i==NULL) {
               struct epoll_event event;
               event.data.fd = ed->fd;
               event.events = EPOLLIN;
               if(read_event(ed->fd)) {
                  _DEBUG("Process: Client closed the connection\n");
                  close(ed->fd);
                  epoll_ctl (epfd, EPOLL_CTL_DEL, ed->fd, &event);
//                  free(ed);
               }
            } else {
               lua_settop(ed->i->L,0);
               //Get the  main coroutine of the instance's handler
               lua_getglobal(ed->i->L, "handler");
               //Put it on the bottom of the instance's stack
               lua_pushboolean(ed->i->L,TRUE);
               //Set the previous number of arguments
               ed->i->args=1;
               push_ready_queue(ed->i);
               struct epoll_event event;
               event.data.fd = ed->fd;
               event.events = EPOLLIN;
               epoll_ctl (epfd, EPOLL_CTL_DEL, ed->fd, &event);
               if(ed->timer==1) 
                  close(ed->fd);
//               free(ed);
            } 
         } else if(events[i].events & EPOLLOUT) {
            ed=events[i].data.ptr;
            if(ed->i!=NULL) {
               lua_settop(ed->i->L,0);
               //Get the  main coroutine of the instance's handler
               lua_getglobal(ed->i->L, "handler");
               //Put it on the bottom of the instance's stack
               lua_pushboolean(ed->i->L,TRUE);
               //Set the previous number of arguments
               ed->i->args=1;
               push_ready_queue(ed->i);
           } 
           struct epoll_event event;
           event.data.fd = ed->fd;
           event.events = EPOLLOUT;
           epoll_ctl (epfd, EPOLL_CTL_DEL, ed->fd, &event);
//           free(ed);
         }
      }
   }
   return NULL;
}

void event_init(int process_fd){
   int *p=malloc(sizeof(int));
   *p=process_fd;
   int i;
   dummy=new_lua_state(FALSE);
   sockets=calloc(main_graph->n_d,sizeof(queue));
   for(i=0;i<main_graph->n_d;i++) sockets[i]=queue_new();
   cur_process=calloc(main_graph->n_cl,sizeof(atomic));
   for(i=0;i<main_graph->n_cl;i++) cur_process[i]=atomic_new(0);
   
   THREAD_CREATE( &event_thread, event_main, p, 0 );
}
