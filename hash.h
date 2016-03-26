#ifndef _LINUX_GHASH_H_
#define _LINUX_GHASH_H_
#include <string.h>

#ifndef __USE_ISOC99
#define inline
#endif
//Tomoka Asagi ÂéÄ¾Ã÷Ïã

#define create_hashtable(hsize) hash_create(lh_strhash, equal_str, hsize)

unsigned int lh_strhash(void *src);
int equal_str(void *k1, void *k2);

struct hashentry;
struct _hashtable;
typedef struct _hashtable hashtable;


hashtable *hash_create(unsigned int (*keyfunc)(void *),int (*comparefunc)(void *,void *),int size);
void hash_free(hashtable *tab);
void hash_insert(void *key, void *data, hashtable *tab);
void hash_remove(void *key, hashtable *tab);
void *hash_value(void *key, hashtable *tab);
void hash_for_each_do(hashtable *tab, int (cb)(void *, void *));
int hash_count(hashtable *tab);

#endif

