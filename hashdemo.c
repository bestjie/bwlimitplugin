#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <time.h>
#include "hash.h"
#include "mysql/include/mysql.h"

#define hash_max 1000000

unsigned char get_value();
int my_rand(int min, int max);
unsigned char get_intbits(int a);
//¹şÏ£
unsigned int hash_func(char* str, unsigned int len)
{
unsigned int hash = 0xAAAAAAAA;
unsigned int i = 0;
for(i = 0; i < len; str++, i++)
{
hash ^= ((i & 1) == 0) ? ( (hash << 7) ^ (*str) * (hash >> 3)) :
(~((hash << 11) + (*str) ^ (hash >> 5)));
}
return hash;
}

int work(void *key, void *data)
{
printf("%s->%s\n",(char *)key, (char *)data);
return 0;
}

int main()
{
int i;

char key[20];
char data[20];

memset(key, 0, 20);
memset(data, 0, 20);
hashtable *tab = create_hashtable(hash_max);
for (i = 0; i <hash_max; i++) {
//srand(time(NULL));
//sprintf(key, "key%d", rand()+i);
sprintf(key, "key%d", i);
sprintf(data, "the line no: %d", i);
hash_insert((void *)strdup(key), (void *)strdup(data), tab);
}

/*printf("remove key4\n");
hash_remove("key4", tab);
printf("key -> value\n");
for (i = 0; i < 10; i++) {
sprintf(key, "key%d", i);
printf("%s->%s\n", key, (char *)hash_value(key, tab));
}
printf("\n");

printf("%s->%s\n", "this is ", (char *)hash_value("this is ", tab));

printf("\n");*/
//hash_for_each_do(tab, work);
printf("current i %d\n", i);
printf("size %d\n", hash_count(tab));
hash_free(tab);
exit(0);
}

unsigned char get_value()
{
if (rand() % 2 == 0)
{
return 1;
}
else
{
return 0;
}
}

int my_rand(int min, int max)
{
return rand();
}

unsigned char get_intbits(int a)
{
int i = 1;
while(1)
{
if (a < 10) return i;
a = (int)a/10;
i++;
}
}