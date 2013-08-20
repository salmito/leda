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
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <lualib.h>
#ifndef ANDROID
#include <sys/timerfd.h>
#endif
#include "event.h"
#include "stats.h"
#include "instance.h"
#include "scheduler.h"
#include "extra/lmarshal.h"
#include "extra/leda-io.h"

#include <event2/event.h>

#include <sys/types.h>
#include <assert.h>
#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define MAXN 1024

struct _element {
   int type; //Type of element
   union  {
      lua_Number n;
      char * str; //for strings
    	bool_t b; //bor booleans
   	void * ptr; //for pointers
   } data;
   size_t len; //Size of data (if type is 'string' it holds the size of string)
};

static THREAD_T event_thread;

/* Free the payload vector of an event.
 * 
 * If an element is a string, it will be freed as well
 */
void destroy_event(event e) {
	_DEBUG("Event: Destroying event");
   size_t i;
   if(e->packed) {
      free(e->data);
      free(e);
      return;
   }
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

#ifdef DEBUG 
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
#endif

/* restore an event to a Lua stack.
 * The event payload is pushed back to the top of the stack
 */
int restore_event_to_lua_state(lua_State * L, event *e_t) {
   size_t i;
   event e=*e_t;
   
   if(e->packed) {
   	_DEBUG("Event: Receiving packed event %p %d\n",e->data,e->data_len);
      int begin=lua_gettop(L);
      lua_getglobal(L,"unpack"); //Push unpack function
      lua_pushcfunction(L,mar_decode);
      
      lua_pushlstring(L,e->data,e->data_len);
      destroy_event(e);
//      dump_stack(L);
      lua_call(L,1,1); //decode event
      lua_call(L,1,LUA_MULTRET); //Unpack event
      return lua_gettop(L)-begin;
   }
   
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
         It has to be deallocated in the event_destroy() function */
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

//Create a packed event
event event_new_packed_event(char * data,size_t len) {
   event e=malloc(sizeof(struct event_data));
   if(!e) return NULL;
   
	e->packed=1; //this event is packed
	e->data=data;
	e->data_len=len;
	return e;
}

/* extract a event from a lua stack of length 'args' from the stack 
 * index 'from' (inclusive).
 */
event extract_event_from_lua_state(lua_State *L, int from, int args) {
   int i,j=0;
   bool_t ok=TRUE;

	event e=malloc(sizeof(struct event_data));
	e->packed=0; //this event is not packed
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

static queue * sockets;
static atomic * cur_process;
//lua_State * dummy;
//Event system base
static struct event_base *base;

void do_read_ack(evutil_socket_t fd, short events, void *arg) {
   _DEBUG("Event: Read ACK\n");
   instance i=arg;
   event_del(i->ev);
   process_id dst_id=i->last_proc;
   char buf[1024];
   size_t size=read(fd,buf,1024);
	   if(size<=0) {
      close(fd);
      lua_settop(i->L,0);
      GET_HANDLER(i->L);
      lua_pushboolean(i->L,FALSE);
      if(size==0) {
       lua_pushfstring(i->L,"Error sending event to process: Process '%s:%d' closed the connection",PROCESS(dst_id)->host,PROCESS(dst_id)->port);
      } else {
         lua_pushfstring(i->L,"Error sending event to process: %s",strerror(errno));
      }
      i->args=2;
      return push_ready_queue(i);
   }
   if(size==1&&buf[0]==1) {
      _DEBUG("Event: Recicling socket %d %p %d\n",(int)dst_id,sockets[dst_id],fd);
      if(!TRY_PUSH(sockets[dst_id],fd)) close(fd);
      lua_settop(i->L,0);
      GET_HANDLER(i->L);
      lua_pushboolean(i->L,TRUE);
      i->args=1;
      _DEBUG("Process: Sent remote event\n");
      if(i->con_id>=0) {
         time_d ct=now_secs()-i->communication_time;
         STATS_UPDATE_EVENTS(i->stage,1,i->con_id,ct*1000000);
      }
      return push_ready_queue(i);
   }
   lua_settop(i->L,0);
   close(fd);
   GET_HANDLER(i->L);
   lua_pushboolean(i->L,FALSE);
   lua_pushfstring(i->L,"Error sending event to process '%s:%d': Event queue is full",PROCESS(dst_id)->host,PROCESS(dst_id)->port);
   i->args=2;
   return push_ready_queue(i);
}

int event_get_process_sock(instance i,process_id dst_id) {
   int sockfd;
   if(!TRY_POP(sockets[dst_id],sockfd)) {
      _DEBUG("Event: Connecting to process '%s:%d'\n",PROCESS(dst_id)->host,PROCESS(dst_id)->port);
      struct sockaddr_in adr_inet;
      adr_inet.sin_family = AF_INET;  
      adr_inet.sin_port = htons(PROCESS(dst_id)->port); 
		long res=0;
		#ifndef _WIN32
			res=inet_aton(PROCESS(dst_id)->host,&adr_inet.sin_addr);
		#else
			res=inet_addr(PROCESS(dst_id)->host);
			adr_inet.sin_addr.s_addr=res;
		#endif
      if (!res ) {
        lua_pop(i->L,1);
        lua_pushboolean(i->L,FALSE);
        lua_pushfstring(i->L,"Bad address '%s'",PROCESS(dst_id)->host);
        return 2;
      }
      if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        lua_pop(i->L,1);
        lua_pushboolean(i->L,FALSE);
        lua_pushliteral(i->L,"Could not create socket");
        return 2;
      }
      if(connect(sockfd, (struct sockaddr *)&adr_inet, sizeof(adr_inet)) < 0) {
        lua_pop(i->L,1);
        lua_pushboolean(i->L,FALSE);
        lua_pushfstring(i->L,"Could not establish connection with process '%s:%d'",PROCESS(dst_id)->host,PROCESS(dst_id)->port);
        return 2;
      }
      evutil_make_socket_nonblocking(sockfd);
      _DEBUG("Event: Connected to process '%s:%d'\n",PROCESS(dst_id)->host,PROCESS(dst_id)->port);
   }
   return sockfd;
}

/* send an event through the network */
int send_async_event(instance i, stage_id s_id, int con_id, time_d communication_time, size_t len ,const char * payload) {
   //cicle through possible destination processes
   int next_d=STORE(cur_process[STAGE(s_id)->cluster],
   (READ(cur_process[STAGE(s_id)->cluster])+1)%CLUSTER(STAGE(s_id)->cluster)->n_processes);
   
   //Request a FD to the target process
   process_id dst_id=CLUSTER(STAGE(s_id)->cluster)->processes[next_d];
   int sockfd=event_get_process_sock(i,dst_id);
   
   size_t header_size=sizeof(stage_id)+sizeof(size_t)+1;
   char * buffer=malloc(len+header_size);
   buffer[0]=EVENT_TYPE;
   size_t h_offset=1;
   memcpy(buffer+h_offset,&s_id,sizeof(stage_id));
   h_offset+=sizeof(stage_id);
   memcpy(buffer+h_offset,&len,sizeof(size_t));
   h_offset+=sizeof(size_t);
   memcpy(buffer+h_offset,payload,len);
   size_t offset=0;
   _DEBUG("Event: Sending event '%zu'\n",len+h_offset);
   while(offset<len+h_offset) {
      _DEBUG("Event: Sending packet %d %p %zu\n",sockfd,buffer+offset,len+h_offset-offset);
      int size=write(sockfd,buffer+offset,len+h_offset-offset);
      if(size<0) {
         if(errno==EAGAIN) {
		_DEBUG("Event: Received EAGAIN\n");
		continue;
	}
         close(sockfd); 
         return size;
      }
      _DEBUG("Event: sent %d\n",size);
      offset+=size;
   }
   free(buffer);
   i->last_proc=dst_id;
   i->con_id=con_id;
   i->communication_time=communication_time;
//   event_base_once(base, sockfd, EV_READ|EV_PERSIST, do_read_ack, i, NULL);
   struct event * ack_event = event_new(base, sockfd, EV_READ | EV_PERSIST, do_read_ack, i);
   i->ev=ack_event;
   event_add(ack_event, NULL);
   return 0;
}

/* send an event through the network and wait for ACK*/
int send_sync_event(lua_State *L) {
   stage_id s_id=lua_tointeger(L,1);

   luaL_checktype(L,-1,LUA_TSTRING);
   size_t len; const char *payload=lua_tolstring(L,-1,&len); 
   lua_pop(L,1);

	//round robin through possible destination processes
   int next_d=STORE(cur_process[STAGE(s_id)->cluster],
   (READ(cur_process[STAGE(s_id)->cluster])+1)%CLUSTER(STAGE(s_id)->cluster)->n_processes);
   
   process_id dst_id=CLUSTER(STAGE(s_id)->cluster)->processes[next_d];
   int sockfd;
   if(!TRY_POP(sockets[dst_id],sockfd)) {
      _DEBUG("Process: Connecting to process '%s:%d'\n",PROCESS(dst_id)->host,PROCESS(dst_id)->port);
      struct sockaddr_in adr_inet;
      adr_inet.sin_family = AF_INET;  
      adr_inet.sin_port = htons(PROCESS(dst_id)->port); 
		long res=0;
		#ifndef _WIN32
			res=inet_aton(PROCESS(dst_id)->host,&adr_inet.sin_addr);
		#else
			res=inet_addr(PROCESS(dst_id)->host);
			adr_inet.sin_addr.s_addr=res;
		#endif
      if (!res ) {
        lua_pop(L,1);
        lua_pushboolean(L,FALSE);
        lua_pushfstring(L,"Bad address '%s'",PROCESS(dst_id)->host);
        return 2;
      }
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
   
   size_t header_size=sizeof(stage_id)+sizeof(size_t)+1;
   char * buffer=malloc(len+header_size);
   buffer[0]=EVENT_TYPE;
   size_t h_offset=1;
   memcpy(buffer+h_offset,&s_id,sizeof(stage_id));
   h_offset+=sizeof(stage_id);
   memcpy(buffer+h_offset,&len,sizeof(size_t));
   h_offset+=sizeof(size_t);
   memcpy(buffer+h_offset,payload,len);
   size_t offset=0;
   _DEBUG("Event: Sending event '%zu'\n",len+h_offset);
   while(offset<len+h_offset) {
      _DEBUG("Event: Sending packet %d %p %zu\n",sockfd,buffer+offset,len+h_offset-offset);
      int size=write(sockfd,buffer+offset,len+h_offset-offset);
      if(size<0) {
         if(errno==EAGAIN) continue;
         close(sockfd); 
         return size;
      }
      _DEBUG("Event: sent %d\n",size);
      offset+=size;
   }
   free(buffer);
   
   char res=0;
  
   int size=read(sockfd,&res,1);
   if(size<=0) {
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
/*   char buf[2048];
   size=read(sockfd,buf,2048);
   if(size<=0) {
      lua_pop(L,1);
      close(sockfd);
      lua_pushboolean(L,FALSE);
      lua_pushfstring(L,"Error7 sending event to process '%s:%d': %*s",PROCESS(dst_id)->host,PROCESS(dst_id)->port,buf,strerror(errno));
      return 2;
   }
   lua_pop(L,1); 
   close(sockfd);*/
   lua_pushboolean(L,FALSE);
   lua_pushfstring(L,"Error sending event to process '%s:%d': %d",PROCESS(dst_id)->host,PROCESS(dst_id)->port,(int)res);
   return 2;
}

/* read an event from a socket ready for reading */
int read_event(int fd) {
   static size_t h_size=sizeof(stage_id)+sizeof(size_t)+1;
   char header[h_size];
   int received=read(fd,header,h_size);
   _DEBUG("Event: Received %d bytes\n",received);
   if(received<=0) {
      _DEBUG("Error receiving event: %s\n",strerror(errno));
      return -1;
   }
   _DEBUG("Event: Received event header: %d bytes\n",received);
   if(header[0]==INIT_TYPE) {
      char ress=4;
      int size=write(fd,&ress,1);
      if(size!=1) return 1;
      _DEBUG("Event: Error: Received init event but the process has already started\n");
      if(size<=0) return -2;
      return -1;
   } else if(header[0]!=EVENT_TYPE) {
      char ress=3;
      int size=write(fd,&ress,1);
      _DEBUG("Event: Error: Data is not an event\n");
      if(size<=0) return -2;
      return -1;
   }
   stage_id s_id;
   size_t len;
   size_t offset=1;
   memcpy(&s_id,header+offset,sizeof(stage_id));
   _DEBUG("Event: Stage id %d\n",(int)s_id);
   offset+=sizeof(stage_id);
   memcpy(&len,header+offset,sizeof(size_t));
   offset+=sizeof(size_t);

   if((int)len<=0) {
      return -1;
   }
   _DEBUG("Event: Receiving event (size=%zu bytes)\n",len);
   char * buf=malloc(len);
   if(!buf) return -1;
   
   int readed=0;
   while(readed<len) {
      int size=read(fd,buf+readed,len-readed);
      if(size<0) {
          if(errno==EAGAIN) continue; //Read socket again
         _DEBUG("Event: Error receiving event (size=%zu bytes): %s\n",len,strerror(errno));
         free(buf);
         return -1;
      }
      if(size > 0) {
         readed+=size;
      } else break;
   }
   
   if(readed!=len) {
      free(buf);
      return 1;
   }
  
   _DEBUG("Event: Event received\n");
   int res=emmit_packed_event(s_id,buf,len);
   if(res==0) {
      char ress=1;
      size_t size=write(fd,&ress,1);
      if(size!=1) return 1;
   } else {
      char ress=2;
      size_t size=write(fd,&ress,1);
      if(size!=1) return 1;
   }
   return 0;
}

int leda_gettime(lua_State * L) {
   lua_pushnumber(L,now_secs());
   return 1;
}

void do_read(evutil_socket_t fd, short events, void *arg) {
	if(read_event(fd)) {
		close(fd);
	}
}

void do_accept(evutil_socket_t listener, short event, void *arg) {
   struct event_base *base = arg;
   struct sockaddr_storage ss;
   socklen_t slen = sizeof(ss);
   int fd = accept(listener, (struct sockaddr*)&ss, &slen);
   if (fd < 0) { // XXXX eagain??
      perror("accept");
   } else if (fd > FD_SETSIZE) {
      close(fd); // XXX replace all closes with EVUTIL_CLOSESOCKET */
   } else {
      evutil_make_socket_nonblocking(fd);
	   struct event * read_event = event_new(base, fd, EV_READ|EV_PERSIST, do_read, base);
      event_add(read_event, NULL);
   }
}



void io_ready(evutil_socket_t fd, short event, void *arg) {
	instance i=(instance)arg;
	lua_settop(i->L,0);
   //Get the  main coroutine of the instance's handler
   GET_HANDLER(i->L);
   //Put it on the bottom of the instance's stack
   lua_pushboolean(i->L,TRUE);
   //Set the number of arguments
 	i->args=1;
   push_ready_queue(i);
}

#ifndef SYNC_IO

#define tofile(L,i)	((FILE **)luaL_checkudata(L, i, LUA_FILEHANDLE))

void event_do_file_aio(instance i) {
	FILE ** f=tofile(i->L,1); //FIXME Error handling
   int fd=fileno(*f);
    
   int mode=-1;
   if (lua_type(i->L,2)==LUA_TNUMBER) {
      mode=lua_tointeger(i->L,2);
   } else {
      lua_settop(i->L,0);
      //Get the  main coroutine of the instance's handler
      GET_HANDLER(i->L);
      //Put it on the bottom of the instance's stack
      lua_pushnil(i->L);
      lua_pushliteral(i->L,"Invalid argument");
      //Set the previous number of arguments
      i->args=2;
      push_ready_queue(i);
      return;
   }

   if(mode==1) {  // read
		int size=-1;
   	if (lua_type(i->L,3)==LUA_TNUMBER) {
   	   size=lua_tointeger(i->L,3);
   	   char * buf=malloc(size);
   	   aio_submit_read(fd, buf, size, i);
   	} else {
      	lua_settop(i->L,0);
      	//Get the  main coroutine of the instance's handler
      	GET_HANDLER(i->L);
      	//Put it on the bottom of the instance's stack
      	lua_pushnil(i->L);
      	lua_pushliteral(i->L,"Invalid argument");
      	//Set the previous number of arguments
      	i->args=2;
      	push_ready_queue(i);
      	return;
   	}
   }
   else if(mode==2) {//write
		size_t size=-1;
   	if (lua_type(i->L,3)==LUA_TSTRING) {
   	   const char * buf=lua_tolstring(i->L,3,&size);
   	   aio_submit_write(fd, buf, size, i);
   	} else {
      	lua_settop(i->L,0);
      	//Get the  main coroutine of the instance's handler
      	GET_HANDLER(i->L);
      	//Put it on the bottom of the instance's stack
      	lua_pushnil(i->L);
      	lua_pushliteral(i->L,"Invalid argument");
      	//Set the previous number of arguments
      	i->args=2;
      	push_ready_queue(i);
      	return;
   	}
   }
   else {
       lua_settop(i->L,0);
       //Get the  main coroutine of the instance's handler
       GET_HANDLER(i->L);
       //Put it on the bottom of the instance's stack
       lua_pushnil(i->L);
       lua_pushliteral(i->L,"Invalid argument");
       //Set the previous number of arguments
       i->args=2;
       push_ready_queue(i);
       return;
   }
}
#define NUM_EVENTS 128

void event_aio_op_ready(evutil_socket_t afd, short libev_event, void *arg) {
	struct io_event events[NUM_EVENTS];
	int j;
	u_int64_t i=0;
	if(read(afd, &i, sizeof(i))!=0) {
	}

	aio_context_t * ctx=arg;
	
	int ev=io_getevents(*ctx, 1, NUM_EVENTS, events, NULL);
	
	for(j=0;j<ev;j++) {
		struct iocb* cb=(struct iocb*)events[j].obj;
		int res=events[j].res;
		instance inst=(instance)cb->aio_data;
		
		if(cb->aio_lio_opcode == IOCB_CMD_PREAD) {
			if(res>0) {
				lseek(cb->aio_fildes,res,SEEK_CUR);
				
				lua_settop(inst->L,0);
		   	//Get the  main coroutine of the instance's handler
   			GET_HANDLER(inst->L);
				lua_pushlstring(inst->L,(const char *)cb->aio_buf,res);
				free((void *)cb->aio_buf);
				free(cb);
		   	//Set the number of arguments
		 		inst->args=1;
		   	push_ready_queue(inst);
//				_DEBUG("Event: Res=%d Bytes=%u offset=%d %p\n",res,(unsigned int)cb->aio_nbytes,(int)cb->aio_offset,inst);
			} else { //if(res==0) {	
				lua_settop(inst->L,0);
		   	//Get the  main coroutine of the instance's handler
   			GET_HANDLER(inst->L);
				lua_pushnil(inst->L);
				lua_pushliteral(inst->L,"EOF");
		 		inst->args=2;
		   	push_ready_queue(inst);
//			_DEBUG("Event: Res=%d Bytes=%u offset=%d %p\n",res,(unsigned int)cb->aio_nbytes,(int)cb->aio_offset,inst);
			}
		} else if(cb->aio_lio_opcode == IOCB_CMD_PWRITE) {
			if(res>0) {
				lseek(cb->aio_fildes,res,SEEK_CUR);
				
				lua_settop(inst->L,0);
		   	//Get the  main coroutine of the instance's handler
   			GET_HANDLER(inst->L);
				lua_pushinteger(inst->L,res);
				free(cb);
		   	//Set the number of arguments
		 		inst->args=1;
		   	push_ready_queue(inst);
//				_DEBUG("Event: Res=%d Bytes=%u offset=%d %p\n",res,(unsigned int)cb->aio_nbytes,(int)cb->aio_offset,inst);
			} else { //if(res==0) {	
				lua_settop(inst->L,0);
		   	//Get the  main coroutine of the instance's handler
   			GET_HANDLER(inst->L);
				lua_pushnil(inst->L);
				lua_pushliteral(inst->L,"Write error");
		 		inst->args=2;
		   	push_ready_queue(inst);
//			_DEBUG("Event: Res=%d Bytes=%u offset=%d %p\n",res,(unsigned int)cb->aio_nbytes,(int)cb->aio_offset,inst);
			}
		}


	}

}
#endif
  
static THREAD_RETURN_T THREAD_CALLCONV event_main(void *t_val) {
   int process_fd=*(int*)t_val;
   free(t_val);

   struct event *listener_event;

   base = event_base_new();
   if (!base) {
   	return NULL; //error
   }

#ifndef SYNC_IO
   {
   aio_context_t *ctx;
	int afd=aio_init(&ctx);
	struct event * aio_ready = event_new(base, afd, EV_READ|EV_PERSIST, event_aio_op_ready, ctx);
   event_add(aio_ready, NULL);
   }
#endif

   listener_event = event_new(base, process_fd, EV_READ|EV_PERSIST, do_accept, base);
   event_add(listener_event, NULL);
   event_base_dispatch(base);
	return NULL;
}

void event_wait_io(instance i) {
   int fd=-1;

   if (lua_type(i->L,1)==LUA_TNUMBER) {
      fd=lua_tointeger(i->L,1);
   } else {
       lua_settop(i->L,0);
       //Get the  main coroutine of the instance's handler
       GET_HANDLER(i->L);
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
      GET_HANDLER(i->L);
      //Put it on the bottom of the instance's stack
      lua_pushnil(i->L);
      lua_pushliteral(i->L,"Invalid argument");
      //Set the previous number of arguments
      i->args=2;
      push_ready_queue(i);
      return;
   }
   int m=0;
   if(mode==1) 
      m = EV_READ; // read
   else if(mode==2)
         m = EV_WRITE; //write
   else if(mode==3)
         m = EV_READ | EV_WRITE; // read & write
   else {
       lua_settop(i->L,0);
       //Get the  main coroutine of the instance's handler
       GET_HANDLER(i->L);
       //Put it on the bottom of the instance's stack
       lua_pushnil(i->L);
       lua_pushliteral(i->L,"Invalid argument");
       //Set the previous number of arguments
       i->args=2;
       push_ready_queue(i);
       return;
   }
   event_base_once(base, fd, m, io_ready, i, NULL);
}



void event_sleep(instance i) {
   double time=0.0l;//now_secs();
   
   if (lua_type(i->L,1)==LUA_TNUMBER) {
      time=lua_tonumber(i->L,1);
   } else {
       lua_settop(i->L,0);
       //Get the  main coroutine of the instance's handler
       GET_HANDLER(i->L);
       //Put it on the bottom of the instance's stack
       lua_pushnil(i->L);
       lua_pushliteral(i->L,"Invalid argument");
       //Set the previous number of arguments
       i->args=2;
       push_ready_queue(i);
       return;
   }

   if(time<0.0L) {
       lua_settop(i->L,0);
       //Get the  main coroutine of the instance's handler
       GET_HANDLER(i->L);
       //Put it on the bottom of the instance's stack
       lua_pushnil(i->L);
       lua_pushliteral(i->L,"Invalid timer");
       //Set the previous number of arguments
       i->args=2;
       push_ready_queue(i);
       return;
   }
   
  	struct timeval to={time,(((double)time-((int)time))*1000000.0)};
  	
	struct event *ev = event_new(base,-1,EV_TIMEOUT,io_ready,i);
   
   //printf("Timer event: %u %u\n",to.tv_sec,to.tv_usec);
   
	if(event_add(ev, &to)) {
   	 lua_settop(i->L,0);
       //Get the  main coroutine of the instance's handler
       GET_HANDLER(i->L);
       //Put it on the bottom of the instance's stack
       lua_pushnil(i->L);
       lua_pushliteral(i->L,"Error setting timer event");
       //Set the previous number of arguments
       i->args=2;
       push_ready_queue(i);
       return;
   }
}

void leda_event_end_t() {
	int i;
	THREAD_KILL(&event_thread);
	event_base_free(base);
	base=NULL;
	for(i=0;i<main_graph->n_d;i++) {
		int * sock;
		while(TRY_POP(sockets[i],sock)) close(*sock);
		queue_free(sockets[i]);
	}
	
   for(i=0;i<main_graph->n_cl;i++) 
   	atomic_free(cur_process[i]);
  	free(sockets);
  	free(cur_process);
}

void event_init_t(int process_fd) {
   int *p=malloc(sizeof(int));
   *p=process_fd;
   int i;
   sockets=calloc(main_graph->n_d,sizeof(queue));
   for(i=0;i<main_graph->n_d;i++) sockets[i]=queue_new();
   cur_process=calloc(main_graph->n_cl,sizeof(atomic));
   for(i=0;i<main_graph->n_cl;i++) cur_process[i]=atomic_new(0);

   THREAD_CREATE( &event_thread, event_main, p, 0 );
}
