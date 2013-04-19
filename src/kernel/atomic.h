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

atomic atomic_new(long int x);
long int atomic_value(atomic x);
long int atomic_fetch_and_store(atomic x, long int y);
long int atomic_fetch_and_add(atomic x, long int y);
long int atomic_compare_and_swap(atomic x, long int y, long int z);
void atomic_free(atomic a);



#ifdef __cplusplus
}
#endif


#endif //_ATOMIC_H_
