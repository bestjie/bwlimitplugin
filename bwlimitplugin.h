#ifndef  BWLIMIT_PLUGIN_H
#define  BWLIMIT_PLUGIN_H

#include "openvpn-plugin.h"



/* bool definitions */
#define bool int
#define true 1
#define false 0
#define CNAME_MAX_LENGTH 32
#define IP_LENGTH  16
#define hash_buckets 1000
#define plugin_legal_argc 2
#define DEFAULT_STRING_LEN 256
#define MAX_CMD_LENGTH 2048
/*shell recode status defines*/
#define RTCODE_SUCCESS 0
#define RTCODE_COMM_PARAM_ERROR 2
#define RTCODE_COMM_SHELL_ERROR  3
#define RTCODE_COMM_PROCESS_ILLEGAL_EXIT 4
#define RTCODE_COMM_SHELL_CMD_FAILED  5
#define NO_FILTER_EXIST   6
/*default tc params*/
#define DEFAULT_BURST 15k
#define DEFAULT_ROOT_QDISC_ID 1
#define DEFAULT_PARENT_CLASS_ID 1
#define DEFAULT_PARENT_DEFAULT_ID 10

#define DEFAULT_TOTAL_RATE 100      //mbps
#define DEFAULT_REST_BW       2048   //kbps
#define DEFAULT_CLIENT_BW   2048  //kbps
//#define SWITCH_ON 1
//#define SWITCH_OFF 0
/*
 * Our context, where we keep our state.
 */

//bw unit is "kbps".

struct plugin_context {
  hashtable * bwlimit_htable;
//  unsigned     switcher;
  unsigned int total_rate;
  unsigned int default_rest_bw;
  unsigned int client_default_bw;
  char *           v_dev;
  unsigned int root_qdisc_id;
  unsigned int parent_class_id;
  unsigned int parent_default_id;
};

struct plugin_per_client_context {
  char cname[CNAME_MAX_LENGTH];
  char client_pool_ip[IP_LENGTH];
  unsigned int client_classid;
};




#endif
