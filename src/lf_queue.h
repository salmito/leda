#ifndef lf_queue_H_
#define lf_queue_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LFqueue * LFqueue_t;

#define leda_lfqueue_trypush(q,p) leda_lfqueue_try_push((q),(void **)&(p))
#define leda_lfqueue_trypop(q,p) leda_lfqueue_try_pop((q),(void **)&(p))

LFqueue_t leda_lfqueue_new();
int leda_lfqueue_try_push(LFqueue_t q,void ** source);
void leda_lfqueue_push(LFqueue_t q,void ** source);
int leda_lfqueue_try_pop(LFqueue_t q, void ** destination);
void leda_lfqueue_pop(LFqueue_t q, void ** destination);
void leda_lfqueue_setcapacity(LFqueue_t q, int capacity);
int leda_lfqueue_getcapacity(LFqueue_t q);
int leda_lfqueue_isempty(LFqueue_t q);
int leda_lfqueue_size(LFqueue_t q);

//possibly thread unsafe
void leda_lfqueue_free(LFqueue_t q);

#ifdef __cplusplus
}
#endif


#endif //lf_queue_H_
