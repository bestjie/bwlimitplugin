/*
 * This file implements a simple OpenVPN plugin module which
 * will apply the function called rate limit,the limit ordered by 
 * different remote ip of every client which was supplied by the
 * vpn  server.
 * 
 * Will run on linux only.
 *
 * Sample usage:
 *

 * plugin .././bwlimitplugin.so   .././bwlimitplugin.cnf
 *

 * See the README file for build instructions.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "hash.h"
#include "bwlimitplugin.h"






/*
 * Given an environmental variable name, search
 * the envp array for its value, returning it
 * if found or NULL otherwise.
 */
static const char *
get_env (const char *name, const char *envp[])
{
  if (envp)
    {
      int i;
      const int namelen = strlen (name);
      for (i = 0; envp[i]; ++i)
	{
	  if (!strncmp (envp[i], name, namelen))
	    {
	      const char *cp = envp[i] + namelen;
	      if (*cp == '=')
		return cp + 1;
	    }
	}
    }
  return NULL;
}

/* used for safe printf of possible NULL strings */
static const char *
np (const char *str)
{
  if (str)
    return str;
  else
    return "[NULL]";
}

static int
atoi_null0 (const char *str)
{
  if (str)
    return atoi (str);
  else
    return 0;
}

int string_array_len ( const char *array[] )
{
	int i = 0;
	if ( array )
	{
		while ( array[i] )
			++i;
	}
	return i;
}

int str_trim(char *buf)
{
    int i = 0;
    int j = 0;
    int len = 0;
    
    if(!buf)
		return -1;

    len = strlen(buf);
    if (len <= 0) {
        return 0;
    }
    
    i = len - 1;
    while (buf[i] == ' ') {
        buf[i--] = '\0';
    }
    len = i + 1;

    while(buf[j] == ' ') {
        j++;
    }
    for (i = j; i < len; i++) {
        buf[i - j] = buf[i];
    }
    buf[i - j] = '\0';
    
    return 0;
}


int iptoid(const char * ip)
{
    char *ip_str[4];
    char ip_buffer[IP_LENGTH];
    char id_buffer[9]="\0";
    char *buffer=ip_buffer;
    strncpy(ip_buffer,ip,IP_LENGTH);
    int i=0;
    while((ip_str[i]=strtok(buffer,"."))!=NULL)
	{
	    i++;
	    buffer=NULL;
	} 
   int j=0;
   for(j;j<4;j++)
	{
	    strncat(id_buffer,ip_str[j],3);
	}
   int flowid=0;
   flowid=atoi_null0(id_buffer);
   flowid%=9999;//classid cannot larger than 9999,so mod 9999
   fprintf(stderr,"parse ip to flow id :%d\n",flowid);
   return flowid;
}

int shell_result_ex(const char *cmd, char *stdout_buf, int stdout_len, 
                        char *stderr_buf, int stderr_len)
{
    int rc = 0,outlen = 0;
//    int status_child;
    FILE *fp = NULL;
    char *p = NULL;
    char cmdx[4096] = {0};
    char shellout[DEFAULT_STRING_LEN * 2];

    if(!cmd)
		return RTCODE_COMM_PARAM_ERROR ;
    snprintf(cmdx,sizeof(cmdx),"%s 2>&1",cmd);//redirect the stderr to stdout
    fprintf(stderr,"excute cmd:'%s'\n",cmd);
    fp = popen(cmdx, "er");
    if (fp == NULL) 
    {
        if (stderr_buf)
	{
	    fprintf(stderr,"shell_result_ex cmd=%s,stderr_buf=%s \n",cmd,stderr_buf);
        }
	else
	{
	    fprintf(stderr,"shell_result_ex cmd=%s error,no error imformation! \n",cmd);
	}
        return RTCODE_COMM_SHELL_ERROR;
    }

    if (stdout_buf)
    {
	  stdout_buf[0] = '\0';
	  while (!feof(fp))
	  {
		    memset(shellout, 0, sizeof(shellout));
		    p = fgets(shellout,sizeof(shellout),fp);
			if (!p)
			{
			    continue;
			}			
			outlen += strlen(p);
			if(outlen < stdout_len)
			    strcat(stdout_buf,shellout);
			else
			    fprintf(stderr,"shell_result_ex cmd=%s,stdout_len=%d is not enough long!\n",cmd,stdout_len);
	 }
        if(stdout_buf[strlen(stdout_buf)-1] == '\n')  //replace the "\n" char with "\0" when there is only one row in the stdout;in case of multi rows,replace the "\n" in the last row
            stdout_buf[strlen(stdout_buf)-1] = 0;	
		str_trim(stdout_buf);
    }
    rc = pclose(fp);
	if(-1 == rc)
	{
	    if (stderr_buf) 
		{
	            snprintf(stderr_buf, sizeof(stderr_len),"Close popen file failed. \n");
		}
	     fprintf(stderr,"shell_result_ex error:Close popen file failed. cmd=%s,with err imformation.\n",cmd);
	    return RTCODE_COMM_PROCESS_ILLEGAL_EXIT;
	}
	/* 解析命令执行结果 */
    if (WIFEXITED(rc)){
        if (WEXITSTATUS(rc)) 
	{
             fprintf(stderr,"'%s' failed, exit code=%d \n", cmd, WEXITSTATUS(rc));
            return RTCODE_COMM_SHELL_CMD_FAILED;
        }
	else
	{
	     return RTCODE_SUCCESS;
	}
    } else {
         fprintf(stderr,"process '%s' exit illegal, exit status=%d\n", cmd, WEXITSTATUS(rc));        
        return RTCODE_COMM_PROCESS_ILLEGAL_EXIT;
    }
}

int find_real_client_bw(struct plugin_context *context,struct plugin_per_client_context *pcc)
{
    int * hash_find_result=NULL;
    int real_client_bw=0;
    hash_find_result=(int *)hash_value(pcc->cname,context->bwlimit_htable);
    if(hash_find_result != NULL)	
       {
           real_client_bw=*hash_find_result;
	    fprintf(stderr,"real_client_bw is:%d kbps\n",real_client_bw);
	}
	else if(context->client_default_bw != 0)
		{
		    real_client_bw=context->client_default_bw;
		     fprintf(stderr,"Do not specifid bwlimit to user :%s,apply the default limit:%d\n",pcc->cname,context->client_default_bw);
		}
	else
		{
		     fprintf(stderr,"Do not specifid bwlimit to user :%s,and default limit is NOT LIMIT!\n ",pcc->cname);
		}
	return real_client_bw;

}

static int 
tc_add_root_qdisc(struct plugin_context *context,const char * v_dev)
{
    int result=0;
    char   cmd_buffer1[DEFAULT_STRING_LEN]="\0";
    char   cmd_stdout_buffer[DEFAULT_STRING_LEN]="\0";

	
    snprintf(cmd_buffer1,DEFAULT_STRING_LEN,"tc qdisc add dev %s root handle %d: htb default %d",\
	    v_dev,context->root_qdisc_id,context->parent_default_id);
    result|=shell_result_ex(cmd_buffer1,cmd_stdout_buffer,DEFAULT_STRING_LEN,NULL,0);
    fprintf(stderr,"%s\n",cmd_stdout_buffer);

    memset(cmd_buffer1,0,DEFAULT_STRING_LEN*sizeof(char));
    memset(cmd_stdout_buffer,0,DEFAULT_STRING_LEN*sizeof(char));
    snprintf(cmd_buffer1,DEFAULT_STRING_LEN,"tc class add dev %s parent %d: classid %d:%d htb rate %dmbps",\
	    v_dev,context->root_qdisc_id,context->root_qdisc_id,context->parent_class_id,context->total_rate);
    result|=shell_result_ex(cmd_buffer1,cmd_stdout_buffer,DEFAULT_STRING_LEN,NULL,0);
    fprintf(stderr,"%s\n",cmd_stdout_buffer);

    memset(cmd_buffer1,0,DEFAULT_STRING_LEN*sizeof(char));
    memset(cmd_stdout_buffer,0,DEFAULT_STRING_LEN*sizeof(char));
    snprintf(cmd_buffer1,DEFAULT_STRING_LEN,"tc class add dev %s parent %d: classid %d:%d htb rate %dkbps",\
    	    v_dev,context->root_qdisc_id,context->root_qdisc_id,context->parent_default_id,context->default_rest_bw);
    result|=shell_result_ex(cmd_buffer1,cmd_stdout_buffer,DEFAULT_STRING_LEN,NULL,0);
    fprintf(stderr,"%s\n",cmd_stdout_buffer);
	
   if(result==0)
   	return OPENVPN_PLUGIN_FUNC_SUCCESS;
   else
   	return OPENVPN_PLUGIN_FUNC_ERROR;
}

static int
tc_clear(struct plugin_context *context, const char * v_dev)
{
    int result=0;
    char   cmd_buffer1[DEFAULT_STRING_LEN]="\0";
    char   cmd_stdout_buffer[DEFAULT_STRING_LEN]="\0";
	
    snprintf(cmd_buffer1,DEFAULT_STRING_LEN,"tc class del dev %s parent %d: classid %d:%d htb rate %dkbps",\
    	    v_dev,context->root_qdisc_id,context->root_qdisc_id,context->parent_default_id,context->default_rest_bw);
    result=shell_result_ex(cmd_buffer1,cmd_stdout_buffer,DEFAULT_STRING_LEN,NULL,0);
    fprintf(stderr,"%s\n",cmd_stdout_buffer);

    memset(cmd_buffer1,0,DEFAULT_STRING_LEN*sizeof(char));
    memset(cmd_stdout_buffer,0,DEFAULT_STRING_LEN*sizeof(char));
    snprintf(cmd_buffer1,DEFAULT_STRING_LEN,"tc class del dev %s parent %d: classid %d:%d htb rate %dmbps",\
	    v_dev,context->root_qdisc_id,context->root_qdisc_id,context->parent_class_id,context->total_rate);
    result|=shell_result_ex(cmd_buffer1,cmd_stdout_buffer,DEFAULT_STRING_LEN,NULL,0);
    fprintf(stderr,"%s\n",cmd_stdout_buffer);

    memset(cmd_buffer1,0,DEFAULT_STRING_LEN*sizeof(char));
    memset(cmd_stdout_buffer,0,DEFAULT_STRING_LEN*sizeof(char));
    snprintf(cmd_buffer1,DEFAULT_STRING_LEN,"tc qdisc del dev %s root handle %d: htb default %d",\
	    v_dev,context->root_qdisc_id,context->parent_default_id);
    result|=shell_result_ex(cmd_buffer1,cmd_stdout_buffer,DEFAULT_STRING_LEN,NULL,0);
    fprintf(stderr,"%s\n",cmd_stdout_buffer);
	
   if(result==0)
   	return OPENVPN_PLUGIN_FUNC_SUCCESS;
   else
   	return OPENVPN_PLUGIN_FUNC_ERROR;

}

static int
tc_add_client_htb_filter(struct plugin_context *context,struct plugin_per_client_context *pcc,const char * client_pool_ip,const char * cname)
{
    int result=0;
    int ret1=0;
    pcc->client_classid=iptoid(pcc->client_pool_ip);
    int client_bw_limit =0;
    if((client_bw_limit = find_real_client_bw(context,pcc)) != 0)
    	{
          char cmd_client_class[DEFAULT_STRING_LEN]="\0";
          char cmd_filter[DEFAULT_STRING_LEN]="\0";
          char   cmd_stdout_buffer[DEFAULT_STRING_LEN]="\0";
          snprintf(cmd_client_class,DEFAULT_STRING_LEN,"tc class add dev %s parent %d:%d classid %d:%d htb rate %dKbps ceil %dkbps burst 15k",  \
	  	    context->v_dev,context->root_qdisc_id,context->parent_class_id,context->parent_class_id,pcc->client_classid,client_bw_limit,client_bw_limit);
          result|=shell_result_ex(cmd_client_class,cmd_stdout_buffer,DEFAULT_STRING_LEN,NULL,0);
          fprintf(stderr,"%s\n",cmd_stdout_buffer);
          if(strcmp(cmd_stdout_buffer,"RTNETLINK answers: File exists")==0)
		  {
		      result=0;
		  }

          memset(cmd_stdout_buffer,0,DEFAULT_STRING_LEN*sizeof(char));
          snprintf(cmd_filter,DEFAULT_STRING_LEN,"tc filter add dev %s parent %d: proto ip prio 1 u32 match ip dst %s/32 flowid %d:%d",\
	  	    context->v_dev,context->root_qdisc_id,client_pool_ip,context->parent_class_id,pcc->client_classid);
          ret1=shell_result_ex(cmd_filter,cmd_stdout_buffer,DEFAULT_STRING_LEN,NULL,0);
          fprintf(stderr,"%s\n",cmd_stdout_buffer);
          if(strcmp(cmd_stdout_buffer,"RTNETLINK answers: File exists")==0)
		  {
		      ret1=0;
		  }
          fprintf(stderr,"%s\n",cmd_stdout_buffer);
          result|=ret1;
	}
   if(result==0)
   	return OPENVPN_PLUGIN_FUNC_SUCCESS;
   else
   	return OPENVPN_PLUGIN_FUNC_ERROR;
}

static int 
tc_del_client_htb_filter(struct plugin_context *context,struct plugin_per_client_context *pcc,const char * client_pool_ip,const char * cname)
{
    int result=0;
    char cmd_client_class[DEFAULT_STRING_LEN]="\0";
    char cmd_filter_list[DEFAULT_STRING_LEN]="\0";
    char cmd_filter[DEFAULT_STRING_LEN]="\0";
    char   cmd_stdout_buffer[DEFAULT_STRING_LEN]="\0";
    char filter_handle[32]="\0";
    int client_bw_limit =0;

    if(client_bw_limit = find_real_client_bw(context,pcc) != 0)
    	{

          snprintf(cmd_filter_list,DEFAULT_STRING_LEN,"tc filter list dev %s |grep \"flowid %d:%d\"|awk \'\{print $10\}\'",\
		      context->v_dev,context->parent_class_id,pcc->client_classid);

          shell_result_ex(cmd_filter_list,filter_handle, DEFAULT_STRING_LEN, NULL, 0);

          if (strlen(filter_handle)<3)
	      {
	        result=NO_FILTER_EXIST;
    	      }
             else 
    	          {
                  snprintf(cmd_filter,DEFAULT_STRING_LEN,"tc filter del dev %s parent %d: protocol ip prio 1 handle %s u32",context->v_dev,context->root_qdisc_id,filter_handle);
                  result=shell_result_ex(cmd_filter,cmd_stdout_buffer, DEFAULT_STRING_LEN, NULL, 0);
                  fprintf(stderr,"%s\n",cmd_stdout_buffer);
    	          }
          memset(cmd_stdout_buffer,0,DEFAULT_STRING_LEN*sizeof(char));
          snprintf(cmd_client_class,DEFAULT_STRING_LEN,"tc class del dev %s parent %d:%d classid %d:%d htb rate %dkbit",  \
	    	      context->v_dev,context->root_qdisc_id,context->parent_class_id,context->parent_class_id,pcc->client_classid,client_bw_limit);
          result|=shell_result_ex(cmd_client_class,cmd_stdout_buffer, DEFAULT_STRING_LEN, NULL, 0);
          fprintf(stderr,"%s\n",cmd_stdout_buffer);
    	}
   if(result==0)
   	return OPENVPN_PLUGIN_FUNC_SUCCESS;
   else
   	return OPENVPN_PLUGIN_FUNC_ERROR;
}

OPENVPN_EXPORT openvpn_plugin_handle_t
openvpn_plugin_open_v2 (unsigned int *type_mask, const char *argv[], const char *envp[], struct openvpn_plugin_string_list **return_list)
{
  struct plugin_context *context;
  struct hashtable * bwlimit_config_hash_t;
  
  bwlimit_config_hash_t=create_hashtable(hash_buckets);
  
   fprintf(stderr,"FUNC: openvpn_plugin_open_v2:bwlimitplugin.so\n");
  /*
    * Allocate our context
    */
  context = (struct plugin_context *) calloc (1, sizeof (struct plugin_context));
  
  context->bwlimit_htable = bwlimit_config_hash_t;
  //context->switcher = SWITCH_ON;
  context->total_rate = DEFAULT_TOTAL_RATE;
  context->default_rest_bw = DEFAULT_REST_BW;
  context->root_qdisc_id= DEFAULT_ROOT_QDISC_ID;
  context->parent_class_id = DEFAULT_PARENT_CLASS_ID;
  context->parent_default_id = DEFAULT_PARENT_DEFAULT_ID;
  context->v_dev="\0";
  
  if(string_array_len(argv) != plugin_legal_argc)
  	{
  	      fprintf(stderr,"unlegal param: %s %s,plz check bwlimitplugin argvs!\n",argv[0],argv[1]);
	     goto error;
  	}
  
  if(parseconfig(argv[1],context ) != 0)
  	{
  	    goto error;
  	}

  /*
   * Which callbacks to intercept.
   */
  *type_mask =
    OPENVPN_PLUGIN_MASK (OPENVPN_PLUGIN_UP) |
    OPENVPN_PLUGIN_MASK (OPENVPN_PLUGIN_DOWN) |
    OPENVPN_PLUGIN_MASK (OPENVPN_PLUGIN_CLIENT_CONNECT_V2) |
    OPENVPN_PLUGIN_MASK (OPENVPN_PLUGIN_CLIENT_DISCONNECT) ;

  return (openvpn_plugin_handle_t) context;
	
error:
		//delete the context
		if (bwlimit_config_hash_t)
			hash_free(bwlimit_config_hash_t);
		if (context )
			free( context );
		return NULL;
}



OPENVPN_EXPORT int
openvpn_plugin_func_v2 (openvpn_plugin_handle_t handle,
			const int type,
			const char *argv[],
			const char *envp[],
			void *per_client_context,
			struct openvpn_plugin_string_list **return_list)
{
  struct plugin_context *context = (struct plugin_context *) handle;
  struct plugin_per_client_context *pcc = (struct plugin_per_client_context *) per_client_context;
  const char *v_dev;
  const char *client_pool_ip;
  const char *cname;
  switch (type)
    {
    case OPENVPN_PLUGIN_UP:
      v_dev = get_env ("dev", envp);
      context->v_dev=v_dev;
      
       fprintf(stderr,"bwlimitplugin.so UP!,v_dev:%s\n",v_dev);
      return tc_add_root_qdisc(context, v_dev);
	  
    case OPENVPN_PLUGIN_DOWN:
      v_dev = get_env ("dev", envp);
      fprintf(stderr,"bwlimitplugin.so DOWN!,v_dev:%s\n",v_dev);
     // return tc_clear(context, v_dev);virtual tun nic has been deleted,no need to manage tc policies anymore.
      return OPENVPN_PLUGIN_FUNC_SUCCESS;	  
    case OPENVPN_PLUGIN_CLIENT_CONNECT_V2:
      client_pool_ip = get_env ("ifconfig_pool_remote_ip", envp);
      cname = get_env ("username",envp);
      strncpy(pcc->cname,cname,CNAME_MAX_LENGTH);
      strncpy(pcc->client_pool_ip,client_pool_ip,IP_LENGTH);
       fprintf(stderr,"OPENVPN_PLUGIN_CLIENT_CONNECT_V2,client_pool_ip : %s ,cname : %s\n",client_pool_ip,cname);
      return tc_add_client_htb_filter(context,pcc,client_pool_ip,cname);
	  
    case OPENVPN_PLUGIN_CLIENT_DISCONNECT:
      client_pool_ip = get_env ("ifconfig_pool_remote_ip", envp);
      cname = get_env ("username",envp);
       fprintf(stderr,"OPENVPN_PLUGIN_CLIENT_DISCONNECT,client_pool_ip : %s ,cname : %s\n",client_pool_ip,cname);
      return tc_del_client_htb_filter(context,pcc,client_pool_ip,cname);
	  

    default:
       fprintf(stderr,"OPENVPN_PLUGIN_?\n");
      return OPENVPN_PLUGIN_FUNC_ERROR;
    }
}

OPENVPN_EXPORT void *
openvpn_plugin_client_constructor_v1 (openvpn_plugin_handle_t handle)
{
  printf ("FUNC: openvpn_plugin_client_constructor_v1\n");
  return calloc (1, sizeof (struct plugin_per_client_context));
}

OPENVPN_EXPORT void
openvpn_plugin_client_destructor_v1 (openvpn_plugin_handle_t handle, void *per_client_context)
{
  printf ("FUNC: openvpn_plugin_client_destructor_v1\n");
  free (per_client_context);
}

OPENVPN_EXPORT void
openvpn_plugin_close_v1 (openvpn_plugin_handle_t handle)
{
  struct plugin_context *context = (struct plugin_context *) handle;
  printf ("FUNC: openvpn_plugin_close_v1\n");
  hash_free(context->bwlimit_htable);
  free (context);
}
