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
#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct atomicInt * atomic;


#define READ(x) atomic_value(x)
#define STORE(x,y) atomic_fetch_and_store((x),(y))
#define ADD(x,y) atomic_fetch_and_add((x),(y))
#define SUB(x,y) atomic_fetch_and_add((x),-(y))
#define SWAP(x,y,z) atomic_compare_and_swap((x),(y),(z))

atomic atomic_new(int x);
int atomic_value(atomic x);
int atomic_fetch_and_store(atomic x, int y);
int atomic_fetch_and_add(atomic x, int y);
int atomic_compare_and_swap(atomic x, int y, int z);
void atomic_free(atomic a);



#ifdef __cplusplus
}
#endif


#endif //_ATOMIC_H_