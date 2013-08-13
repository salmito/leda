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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef _WIN32
#include <sys/socket.h>
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "leda-io.h"

#define tofile(L,i)	((FILE **)luaL_checkudata(L, i, LUA_FILEHANDLE))

int leda_wrap_io(lua_State *L) {  
   FILE ** f=tofile(L,1);
//   int fd=fileno(*f);
//   int newfd=dup(fd);
   lua_pushlightuserdata(L,*f);
 	*f=NULL;
   return 1;
}

int leda_io_getfd(lua_State *L) {
   FILE ** f=tofile(L,1);
   int fd=fileno(*f);
   lua_pushinteger(L,fd);
   return 1;
}

int leda_unwrap_io(lua_State *L) {
	if(!lua_islightuserdata(L,1)) {
		luaL_error(L,"Invalid parameter #1");
	}

   FILE **f = (FILE **)lua_newuserdata(L, sizeof(FILE *));
	*f=(FILE *) lua_touserdata (L, 1);
	luaL_getmetatable(L, LUA_FILEHANDLE);
   lua_setmetatable(L, -2);
	
   return (*f != NULL);
}

int socket_flush(lua_State *L) {
	#ifndef _WIN32
   int fd=lua_tointeger(L,1);
   int nfd=dup(fd);
   shutdown(nfd,SHUT_RDWR);
   close(nfd);
	#else
		lua_pushliteral(L,"Not implemented");
		lua_error(L);
	#endif
   return 0;
}

//AIO interface

#ifndef SYNC_IO

#include <sys/syscall.h>
#include <sys/eventfd.h>
//#include <sys/signal.h>
#include <sys/time.h>
#include <sys/uio.h>

//#include <string.h>
#include <signal.h>
//#include <poll.h>
#include <fcntl.h>
#include <time.h>
//#include <errno.h>


/*#ifndef __NR_eventfd
#if defined(__x86_64__)
#define __NR_eventfd 284
#elif defined(__i386__)
#define __NR_eventfd 323
#else
#error Cannot detect your architecture!
#endif
#endif*/

#define IOCB_FLAG_RESFD		(1 << 0)


/*static void asyio_prep_preadv(struct iocb *iocb, int fd, struct iovec *iov,
			      int nr_segs, int64_t offset, int afd, void * data)
{
	memset(iocb, 0, sizeof(*iocb));
	iocb->aio_data = (u_int64_t)data;
	iocb->aio_fildes = fd;
	iocb->aio_lio_opcode = IOCB_CMD_PREADV;
	iocb->aio_reqprio = 0;
	iocb->aio_buf = (u_int64_t) iov;
	iocb->aio_nbytes = nr_segs;
	iocb->aio_offset = offset;
	iocb->aio_flags = IOCB_FLAG_RESFD;
	iocb->aio_resfd = afd;
}

static void asyio_prep_pwritev(struct iocb *iocb, int fd, struct iovec *iov,
			       int nr_segs, int64_t offset, int afd,void * data)
{
	memset(iocb, 0, sizeof(*iocb));
	iocb->aio_data = (u_int64_t)data;
	iocb->aio_fildes = fd;
	iocb->aio_lio_opcode = IOCB_CMD_PWRITEV;
	iocb->aio_reqprio = 0;
	iocb->aio_buf = (u_int64_t) iov;
	iocb->aio_nbytes = nr_segs;
	iocb->aio_offset = offset;
	iocb->aio_flags = IOCB_FLAG_RESFD;
	iocb->aio_resfd = afd;
}*/

static void asyio_prep_pread(struct iocb *iocb, int fd, void *buf,
			     int nr_segs, int64_t offset, int afd,void * data)
{
	memset(iocb, 0, sizeof(*iocb));
	iocb->aio_data = (u_int64_t)data;
	iocb->aio_fildes = fd;
	iocb->aio_lio_opcode = IOCB_CMD_PREAD;
	iocb->aio_reqprio = 0;
	iocb->aio_buf = (u_int64_t) buf;
	iocb->aio_nbytes = nr_segs;
	iocb->aio_offset = offset;
	iocb->aio_flags = IOCB_FLAG_RESFD;
	iocb->aio_resfd = afd;
}

static void asyio_prep_pwrite(struct iocb *iocb, int fd, void const *buf,
			      int nr_segs, int64_t offset, int afd, void * data)
{
	memset(iocb, 0, sizeof(*iocb));
	iocb->aio_data = (u_int64_t)data;
	iocb->aio_fildes = fd;
	iocb->aio_lio_opcode = IOCB_CMD_PWRITE;
	iocb->aio_reqprio = 0;
	iocb->aio_buf = (u_int64_t) buf;
	iocb->aio_nbytes = nr_segs;
	iocb->aio_offset = offset;
	iocb->aio_flags = IOCB_FLAG_RESFD;
	iocb->aio_resfd = afd;
}

static long io_setup(unsigned nr_reqs, aio_context_t *ctx) {
	return syscall(__NR_io_setup, nr_reqs, ctx);
}

long io_destroy(aio_context_t ctx) {
	return syscall(__NR_io_destroy, ctx);
}

long io_submit(aio_context_t ctx, long n, struct iocb **paiocb) {
	return syscall(__NR_io_submit, ctx, n, paiocb);
}

long io_cancel(aio_context_t ctx, struct iocb *aiocb,
		      struct io_event *res) {
	return syscall(__NR_io_cancel, ctx, aiocb, res);
}

long io_getevents(aio_context_t ctx, long min_nr, long nr,
			 struct io_event *events, struct timespec *tmo) {
	return syscall(__NR_io_getevents, ctx, min_nr, nr, events, tmo);
}

int aio_eventfd(unsigned int count) {
	return eventfd(count, 0);
}

static int afd;
static aio_context_t ctx = 0;

#define MAX_AIO_EVENTS 1024*4
#define AIO_EVENTS 512

int leda_aio_init(aio_context_t ** ctx_p) {
//	fprintf(stderr,"Creating a fdevent\n");
	if ((afd = aio_eventfd(0)) == -1) {
		return 2;
	}
	if (io_setup(MAX_AIO_EVENTS, &ctx)) {
		return 3;
	}
	fcntl(afd, F_SETFL, fcntl(afd, F_GETFL, 0) | O_NONBLOCK);
	*ctx_p=&ctx;
	return afd;
}

void leda_aio_end() {
	close(afd);
	ctx=0;
}

int aio_submit_read(int fd, char * buf, int size, void * data) {
	int i, r;// j;
	struct iocb **piocb;
	struct iocb *iocb;

	iocb = malloc(sizeof(struct iocb));
	piocb = malloc(sizeof(struct iocb *));
	if (!iocb || !piocb) {
		return -1;
	}
	i=0;
	off_t offset = lseek( fd, 0, SEEK_CUR );
		piocb[i] = &iocb[i];
		asyio_prep_pread(&iocb[i], fd, buf, size,
				 offset, afd, data);
	if ((r = io_submit(ctx, 1, piocb)) <= 0) {
		return -1;
	}
	return r;
}

int aio_submit_write(int fd, const char * buf,int size,void * data) {
	int i, r;

	struct iocb **piocb;
	struct iocb *iocb;

	iocb = malloc(sizeof(struct iocb));
	piocb = malloc(sizeof(struct iocb *));
	if (!iocb || !piocb) {
		perror("iocb alloc");
		return -1;
	}
	i=0;
	off_t offset = lseek( fd, 0, SEEK_CUR ) ;
		piocb[i] = &iocb[i];
		asyio_prep_pwrite(&iocb[i], fd, buf, size,
				  offset, afd,data);
	if ((r = io_submit(ctx, 1, piocb)) <= 0) {
		return -1;
	}
	return r;
}


#endif
