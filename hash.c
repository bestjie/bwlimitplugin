#include <string.h>
#ifdef DMALLOC
#include <dmalloc.h>
#else
#include <stdlib.h>
#endif
#include "hash.h"
#ifndef __USE_ISOC99
#define inline
#endif

struct hashentry
{
    void *key;
    void *data;
    struct hashentry *next;
};

struct _hashtable
{
    unsigned int (*gethash)(void *);
    int (*compare)(void *, void *);
    int hashsize;
    int count;
    struct hashentry **hashlist;
};

#define hashindex(key, tab) ((tab->gethash)(key) % (tab->hashsize -1))

unsigned int lh_strhash(void *src)
{
    int i, l;
    unsigned long ret = 0;
    unsigned short *s;
    char *str = (char *)src;
    if (str == NULL)
        return(0);
    l = (strlen(str) + 1) / 2;
    s = (unsigned short *)str;

    for (i = 0; i < l; i++)
        ret ^= s[i]<<(i&0x0f);

    return(ret);
}

int equal_str(void *k1, void *k2)
{
    return (0 == strcmp((char *)k1, (char *)k2));
}

inline struct hashentry *hashentry_new(void *key, void *data)
{
    struct hashentry *new = malloc(sizeof(struct hashentry));
    new->key = key;
    new->data = data;
    new->next = NULL;
    return new;
}

void hlist_append(struct hashentry **root, void *key, void *data)
{
    struct hashentry *l, *pos;
    l = hashentry_new(key, data);
    if (*root == NULL) {
        *root = l;
    } else {
        for(pos = *root; pos->next != NULL; pos = pos->next);
            pos->next = l;
    }
}

int hlist_update(struct hashentry *root, void *key, void *data,
        int (*compare)(void *, void *))
{
    struct hashentry *pos;
    for(pos = root; pos != NULL; pos = pos->next ) {
        if ( compare(key, pos->key) ) {
            free(pos->data);
            pos->data = data;
            free(key);
            return 0;
        }
    }
    return -1;
}

inline struct hashentry *hashentry_free(struct hashentry *h)
{
    struct hashentry *next = h->next;
    free(h->key);
    free(h->data);
    free(h);
    h = NULL;
    return (next);
}

int hlist_remove(struct hashentry **root, void *key,
                 int (*compare)(void *,void *))
{
    struct hashentry *pos ,*prev;

    if (NULL == *root) return -1;

    if (compare((*root)->key, key)) {
        *root = hashentry_free(*root);
        return 0;
    }

    prev = *root;
    for (pos = prev->next; NULL != pos; pos = pos->next) {
        if (compare(pos->key, key)) {
            prev->next = hashentry_free(pos);
            return 0;
        }
        prev = pos;
    }
    return -1;
}

hashtable *hash_create(unsigned int (*keyfunc)(void *),
                       int (*comparefunc)(void *, void *),
                       int size)
{
    int len = sizeof(struct hashentry *) * size;
    int i;
    hashtable *tab = malloc( sizeof(hashtable) );
    memset(tab, 0, sizeof(hashtable));
    tab->hashlist = malloc(len);

    if (tab->hashlist == NULL) {
        free(tab);
        return NULL;
    }

    memset(tab->hashlist, 0, len );
    for (i = 0; i < size; i++)
        tab->hashlist[i] = NULL ;

    tab->compare = comparefunc;
    tab->gethash = keyfunc;
    tab->hashsize = size;
    tab->count = 0;
    return tab;
}

void hash_free(hashtable *tab)
{
    int i;
    struct hashentry *pos;

    for (i = 0; i < tab->hashsize; i++)
        for (pos = tab->hashlist[i]; NULL != pos; pos = hashentry_free(pos));

    free(tab->hashlist);
    free(tab);
    tab =NULL;
}

void hash_insert(void *key, void *data, hashtable *tab)
{
    unsigned int index = hashindex(key, tab);
    struct hashentry *root = tab->hashlist[index];

    if ( hlist_update(root, key, data, tab->compare ) != 0 ) { //(1)

        hlist_append(&(tab->hashlist[index]), key, data );
        tab->count++;
    }
}

//(1) 查看Hash Table中是否存在键值为key的项，如果有则替换该键值所对应的value，否则调用hlist_append为key, data生成新的hashentry并插入相应的队列中。


void hash_remove(void *key, hashtable *tab)
{
    unsigned int index = hashindex(key, tab);
    if (hlist_remove(&(tab->hashlist[index]), key, tab->compare) == 0) {
        tab->count--;
    }
}

void *hash_value(void *key, hashtable *tab)
{
    struct hashentry *pos;
    unsigned int index = hashindex(key, tab);
    for (pos = tab->hashlist[index]; NULL != pos; pos = pos->next) {
        if (tab->compare(key, pos->key)) {
            return (pos->data);
        }
    }
    return NULL;
}


void hash_for_each_do(hashtable *tab, int(cb)(void *, void *))
{
    int i = 0;
    struct hashentry *pos;
    for (i = 0; i < tab->hashsize; i++) {
        for (pos = tab->hashlist[i]; NULL != pos; pos = pos->next ) {
            cb(pos->key, pos->data);
        }
    }
}

inline int hash_count(hashtable *tab)
{
    return tab->count;
}