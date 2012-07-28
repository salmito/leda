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

#include "tbb/atomic.h"

//Currently not being used for anything

extern "C" {

struct atomicInt {
   tbb::atomic<int> i;
};

atomic atomic_new(int i) {
   atomic a;
   a=new atomicInt();
   a->i=i;
   return a;
}

int atomic_value(atomic x) {
   return x->i;
}

/*do x=y and return the old value of x*/
int atomic_fetch_and_store(atomic x, int y) {
   return x->i.fetch_and_store(y);
}

/*do x+=y and return the old value of x*/
int atomic_fetch_and_add(atomic x, int y) {
   return x->i.fetch_and_add(y);
}

/* if x equals z, then do x=y.
   In either case, return old value of x. */
int atomic_compare_and_swap(atomic x, int y, int z) {
   return x->i.compare_and_swap(y,z);
}

/*deallocate atomic pointer*/
void atomic_free(atomic a) {
   if(a)
      delete (a);
}

} //extern "C"




