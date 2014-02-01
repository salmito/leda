#ifndef lf_queue_H_
#define lf_queue_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LFqueue * LFqueue_t;

#define lstage_lfqueue_trypush(q,p) lstage_lfqueue_try_push((q),(void **)&(p))
#define lstage_lfqueue_trypop(q,p) lstage_lfqueue_try_pop((q),(void **)&(p))

LFqueue_t lstage_lfqueue_new();
int lstage_lfqueue_try_push(LFqueue_t q,void ** source);
void lstage_lfqueue_push(LFqueue_t q,void ** source);
int lstage_lfqueue_try_pop(LFqueue_t q, void ** destination);
void lstage_lfqueue_pop(LFqueue_t q, void ** destination);
void lstage_lfqueue_setcapacity(LFqueue_t q, int capacity);
int lstage_lfqueue_getcapacity(LFqueue_t q);
int lstage_lfqueue_isempty(LFqueue_t q);
int lstage_lfqueue_size(LFqueue_t q);

//possibly thread unsafe
void lstage_lfqueue_free(LFqueue_t q);

#ifdef __cplusplus
}
#endif


#endif //lf_queue_H_
