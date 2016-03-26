#ifndef  BWLIMIT_CONFIG_H
#define  BWLIMIT_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "hash.h"
#include "bwlimitplugin.h"

#define  max_length_cname  64
#define LINE_BUFF_LENGTH 256
#define MAX_PARAS 3

#define BOOL_CAST(x) ((x) ? (true) : (false))

/* size of an array */
#define SIZE(x) (sizeof(x)/sizeof(x[0]))

/* clear an object */
#define CLEAR(x) memset(&(x), 0, sizeof(x))



 struct bwlimit
{
   char  cname[max_length_cname];
   unsigned int   rate;
};


int parseconfig(const char * config_file,struct plugin_context * context);


#endif

