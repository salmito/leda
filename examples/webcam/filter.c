#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

typedef unsigned char uint8;

//#include <pthread.h>

//extern pthread_mutex_t mutex;

static int free_frame(lua_State *L) {
	void *ptr;
	if(lua_type(L,1)!=LUA_TLIGHTUSERDATA) {
   	lua_pushboolean(L,0);
		lua_pushliteral(L,"Arg is not a pointer");
		return 2;
	}
	ptr=lua_touserdata(L,1);
	if(ptr) {
		free(ptr);
	}
 	lua_pushboolean(L,1);
	return 1;
}

static int copy_frame(lua_State *L) {
	void *ptr;
	if(lua_type(L,1)!=LUA_TLIGHTUSERDATA) {
   	lua_pushboolean(L,0);
		lua_pushliteral(L,"Arg is not a pointer");
		return 2;
	}
	ptr=lua_touserdata(L,1);
	if(ptr) {
      int i,n,w,h;
      w=lua_tointeger(L,2);
      h=lua_tointeger(L,3);
      if(lua_type(L,4)==LUA_TNUMBER) 
         n=lua_tointeger(L,1);
      else
         n=1;
      for(i=0;i<n;i++) {
         char * copy=malloc(w*h*3);
         memcpy(copy,ptr,w*h*3);
         lua_pushlightuserdata(L,copy);
      }
      return n;
	}
 	lua_pushboolean(L,0);
	lua_pushliteral(L,"Frame pointer is NULL");
	return 2;
}

static int negativo3(lua_State *L) {
   unsigned char* buf;
	if(lua_type(L,1)!=LUA_TLIGHTUSERDATA) {
   	lua_pushboolean(L,0);
		lua_pushliteral(L,"Arg is not a pointer");
		return 2;
	}
	buf=lua_touserdata(L,1);
	if(buf) {
	   printf("negating\n");
      int w,h,pix;
      w=lua_tointeger(L,2);
      h=lua_tointeger(L,3);
   	unsigned char * r, * b, * g;
   	r=buf;
   	g=buf+1;
   	b=buf+2;
	   for(pix=0;pix<h*w;pix++) {
	   	*r=255-*r;
	   	*g=255-*g;
	   	*b=255-*b;
	   	r+=3;
	   	g+=3;
	   	b+=3;
	   }
	}
	return 0;
}

int equaliza(lua_State * L);
int dourado(lua_State *L);

int LUA_API luaopen_filter(lua_State *L)
{
    const luaL_Reg meth[] = 
    {
        {"free", free_frame},
        {"copy", copy_frame},
        {"neg", negativo3},
        {"equalize", equaliza},
        {"gold", dourado},
        {NULL, NULL},
    };
    
    luaL_openlib (L, "filter", meth, 0);
    lua_settable(L, -1);
 
    return 1;
}

void histograma(unsigned char * buf,int * hist, int w, int h, int rgb) {
    int y,x;
	 memset(hist,0,256*sizeof(int));
	 unsigned char * tmp;
	 
  	 for(y=0;y<h;y++) for(x=0;x<w;x++) {
	    tmp=&buf[(x*3 + y*w*3)];
		 int media = tmp[rgb];// + tmp[ 1 ] + tmp[ 2 ];
  		 hist[media]++;
     }    
}

double Cumulativo(int * hist,int i, int w, int h) {
	double soma=0;
	int j;
	for(j=0;j<=i;j++) {
		soma+=(double)(hist[j]);
	}
	return soma;
}

void printhist(int * hist,int w,int h) {
	 int soma=0,i;
	 for(i=0;i<256;i++) { soma+=hist[i]; printf("hist[%d]=%d\n",i,hist[i]); }	 
	 printf("soma=%d n=%d\n",soma,w*h);
}

int equaliza(lua_State * L) {
   unsigned char* buf;
	if(lua_type(L,1)!=LUA_TLIGHTUSERDATA) {
   	lua_pushboolean(L,0);
		lua_pushliteral(L,"Arg is not a pointer");
	   return 2;
	}
	buf=lua_touserdata(L,1);
	if(!buf) {
	   return 0;
	}
   int w,h;
   w=lua_tointeger(L,2);
   h=lua_tointeger(L,3);
   int hist[256],rgb,x,y;
	 for(rgb=0;rgb<3;rgb++) {
  	 histograma(buf,hist,w,h,rgb);

	 unsigned char * tmp;
	 int val;
	 double c;
	 
	 double i=(w*h)/255.0;
	 
	 //printhist(hist,w,h);
	 
  	 for(y=0;y<h;y++) for(x=0;x<w;x++) {
 		tmp=&buf[(x*3 + y*w*3)];
 		val = tmp[rgb];// + tmp[ 1 ] + tmp[ 2 ];
		c=Cumulativo(hist,val,w,h);
	 	tmp[rgb] = (unsigned char)(c/i);
     }
	 }
	 lua_pushboolean(L,1);
	 return 1;
}

#define PI 3.141592653589793238462643

int dourado(lua_State *L) {
   unsigned char * buf;
   int n=3,w,h,i,j;
   
	if(lua_type(L,1)!=LUA_TLIGHTUSERDATA) {
   	lua_pushboolean(L,0);
		lua_pushliteral(L,"Arg is not a pointer");
	   return 2;
	}
	buf=lua_touserdata(L,1);
	if(!buf) {
	   return 0;
	}
   w=lua_tointeger(L,2);
   h=lua_tointeger(L,3);


	double tmpx,tmpy,angulo,tmp;
	for(j=0;j<h;j++) for(i=0;i<w;i++) {
	    unsigned char * val=&buf[(i*3 + j*w*3)];
	    unsigned char * valout=val;
	    tmpx=((n*2)+1)*PI*(*val)/(2*255);
	    //printf("tmp=%f\n",tmp);
	    tmp=255.0*sin(tmpx);
	    if(tmp<0.0) tmp=-1.0*tmp;
 		//printf("tmp2=%f\n",tmp);
 		//	printf("aqui %d %d (%dx%d)\n",i,j,w,h);
		if(tmp<55.0) { //primeiro caso
			*valout=(unsigned char)(3.45454*tmp);	
			*(valout+1)=(unsigned char)(tmp);	 
			*(valout+2)=0;
		} else if(tmp<155.0) { //segundo caso
 		    *valout=(unsigned char)(0.65*tmp+154.25);
 		    *(valout+1)=(unsigned char)(1.35*tmp-19.25);	 
 		    *(valout+2)=(unsigned char)(0.5*tmp-27.5);
		} else { //terceiro caso
		  *valout=255;
          *(valout+1)=(unsigned char)(0.65*tmp+89.25);
          *(valout+2)=(unsigned char)(2.05*tmp-267.75);
		}
   }
		 lua_pushboolean(L,1);
	 return 1;
}

