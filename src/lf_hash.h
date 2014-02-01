#ifndef LF_HASH_H
#define LF_HASH_H


typedef const void *qt_key_t;
typedef struct qt_hash_s *qt_hash;
typedef void (*qt_hash_callback_fn)(const qt_key_t, void *, void *);
typedef void (*qt_hash_deallocator_fn)(void *);
int qt_hash_put(qt_hash  h, qt_key_t key,void *value);                
void *qt_hash_get(qt_hash h, const qt_key_t key);
int qt_hash_remove(qt_hash h, const qt_key_t key);
qt_hash qt_hash_create();
void qt_hash_destroy(qt_hash h);
void qt_hash_destroy_deallocate(qt_hash h,qt_hash_deallocator_fn f);
size_t qt_hash_count(qt_hash h);
void qt_hash_callback(qt_hash h, qt_hash_callback_fn f, void *arg);
void print_ent(const qt_key_t k, void *v, void *a);

#endif
