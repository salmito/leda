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
#include <stdio.h>

#include "atomic.h"
#include "thread.h"

struct atomicInt {
   long int i;
   MUTEX_T lock;
};

atomic atomic_new(long int x) {
   atomic a;
   a=malloc(sizeof(struct atomicInt));
   a->i=x;
   MUTEX_INIT(&a->lock);
   return a;
}

long int atomic_value(atomic x) {
//   MUTEX_LOCK(&x->lock);
   //long int oldx=x->i;
//   MUTEX_UNLOCK(&x->lock);
//   return oldx;
   return x->i;
}

/*do x=y and return the old value of x*/
long int atomic_fetch_and_store(atomic x, long int y) {
   MUTEX_LOCK(&x->lock);
   long int oldx=x->i;
   x->i=y;
   MUTEX_UNLOCK(&x->lock);
   return oldx;
}

/*do x+=y and return the old value of x*/
long int atomic_fetch_and_add(atomic x, long int y) {
   MUTEX_LOCK(&x->lock);
   long int oldx=x->i;
   x->i+=y;
   MUTEX_UNLOCK(&x->lock);
   return oldx;
}

/* if x equals z, then do x=y.
   In either case, return old value of x. */
long int atomic_compare_and_swap(atomic x, long int y, long int z) {
   MUTEX_LOCK(&x->lock);
   long int oldx=x->i;
   if(oldx==z)
      x->i=y;
   MUTEX_UNLOCK(&x->lock);
   return oldx;
}

/*deallocate atomic polong inter*/
void atomic_free(atomic a) {
   MUTEX_FREE(&a->lock);
   free(a);
}
