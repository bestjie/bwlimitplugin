#include "config.h"


static inline bool
space (unsigned char c)
{
  return c == '\0' || isspace (c);
}



/*
@const int n:
*/
int
parse_line (const char *line,
	    char *p[],
	    const int n,
	    const char *file,
	    const int line_num)
{
  const int STATE_INITIAL = 0;
  const int STATE_READING_QUOTED_PARM = 1;
  const int STATE_READING_UNQUOTED_PARM = 2;
  const int STATE_DONE = 3;
  const int STATE_READING_SQUOTED_PARM = 4;

  const char *error_prefix = "";

  int ret = 0;
  const char *c = line;
  int state = STATE_INITIAL;
  bool backslash = false;
  char in, out;

  char parm[LINE_BUFF_LENGTH];
  unsigned int parm_len = 0;

  do
    {
      in = *c;
      out = 0;

      if (!backslash && in == '\\' && state != STATE_READING_SQUOTED_PARM)
	{
	  backslash = true;
	}
      else
	{
	  if (state == STATE_INITIAL)
	    {
	      if (!space (in))
		{
		  if (in == ';' || in == '#') /* comment */
		    break;
		  if (!backslash && in == '\"')
		    state = STATE_READING_QUOTED_PARM;
		  else if (!backslash && in == '\'')
		    state = STATE_READING_SQUOTED_PARM;
		  else
		    {
		      out = in;
		      state = STATE_READING_UNQUOTED_PARM;
		    }
		}
	    }
	  else if (state == STATE_READING_UNQUOTED_PARM)
	    {
	      if (!backslash && space (in))
		state = STATE_DONE;
	      else
		out = in;
	    }
	  else if (state == STATE_READING_QUOTED_PARM)
	    {
	      if (!backslash && in == '\"')
		state = STATE_DONE;
	      else
		out = in;
	    }
	  else if (state == STATE_READING_SQUOTED_PARM)
	    {
	      if (in == '\'')
	        state = STATE_DONE;
	      else
	        out = in;
	    }
	  if (state == STATE_DONE)
	    {
	      /* ASSERT (parm_len > 0); */
	      p[ret] = malloc (parm_len + 1);
	      memset(p[ret],0,parm_len+1);
	      memcpy (p[ret], parm, parm_len);
	      p[ret][parm_len] = '\0';
	      state = STATE_INITIAL;
	      parm_len = 0;
	      ++ret;
	    }

	  if (backslash && out)
	    {
	      if (!(out == '\\' || out == '\"' || space (out)))
		{
		  return 0;
		}
	    }
	  backslash = false;
	}

      /* store parameter character */
      if (out)
	{
	  if (parm_len >= SIZE (parm))
	    {
	      parm[SIZE (parm) - 1] = 0;
	       fprintf(stderr,"%sOptions error: Parameter at %s:%d is too long (%d chars max): %s",error_prefix, file, line_num, (int) SIZE (parm), parm);
	      return 0;
	    }
	  parm[parm_len++] = out;
	}

      /* avoid overflow if too many parms in one config file line */
      if (ret >= n)
	break;

    } while (*c++ != '\0');

  if (state == STATE_READING_QUOTED_PARM)
    {
       fprintf(stderr,"%sOptions error: No closing quotation (\") in %s:%d", error_prefix, file, line_num);
      return 0;
    }
  if (state == STATE_READING_SQUOTED_PARM)
    {
       fprintf(stderr,"%sOptions error: No closing single quotation (\') in %s:%d", error_prefix, file, line_num);
      return 0;
    }
  if (state != STATE_INITIAL)
    {
       fprintf(stderr,"%sOptions error: Residual parse state (%d) in %s:%d", error_prefix, state, file, line_num);
      return 0;
    }
    return ret;
}

int parseconfig(const char * config_file,struct plugin_context * context)
{
    FILE *fp;
    unsigned int line_num=0;
    int i=0;
    char * p[MAX_PARAS];
    char line_buff[LINE_BUFF_LENGTH];
    fp=fopen(config_file,"r");
    if(fp)
	{
	    line_num = 0;
	    while (fgets(line_buff, sizeof (line_buff), fp))
	    {
	      CLEAR(p);
	      ++line_num;
	      if (parse_line (line_buff, p, SIZE (p), config_file, line_num))
		{
		  if(strcmp(p[0],"default")==0)
		  	{
		  	    if(p[1])
				{
				     context->client_default_bw=atoi(p[1]);
				     fprintf(stderr,"default bw apply to:%d Kbps\n",context->client_default_bw);
				}
			    else
				{
				    context->client_default_bw=DEFAULT_CLIENT_BW;
				     fprintf(stderr,"default bw for unspecified client is null,apply to default:%d Kbps\n",DEFAULT_CLIENT_BW);
				}
		  	}
		  else if(strcmp(p[0],"total")==0)
		  	{
		  	    if(p[1])
				{
				     context->total_rate=atoi(p[1]);
				     fprintf(stderr,"Total rate  apply to:%d Mbps\n",context->total_rate);
				}
			    else
				{
				    context->total_rate=DEFAULT_TOTAL_RATE;
				     fprintf(stderr,"total bw for all clients is NULL,apply to default:%d Mbps\n",DEFAULT_TOTAL_RATE);
				}
		  	}
		  else if(strcmp(p[0],"user")==0)
		  	{
		  	    if(p[1]&&p[2])
				{
				     char *cname=malloc(CNAME_MAX_LENGTH*sizeof(char));
				     int    *client_bw=malloc(sizeof(int));
				     strncpy(cname,p[1],CNAME_MAX_LENGTH);
				     *client_bw=atoi(p[2]);
				     hash_insert(cname,client_bw,context->bwlimit_htable);
				     fprintf(stderr,"user:%s  bw apply to:%d Kbps\n",cname,*client_bw);
				}
				else 
				{
				     fprintf(stderr,"unexpected charters after keywork \"user\", abandon this line %d !\n",line_num);
				}
		  	}
		  else 
		  	{
		  		 fprintf(stderr,"Unrecognized option or missing parameter(s): %s\n", p[0]);

		  	}
		  for(i;i<MAX_PARAS;i++)
		  	{
		  	    if(p[i] != NULL)
				{
			            free(p[i]);
				}
		  	}
		}
	    }
	    close(fp);
	}
    else
    	{
    	     fprintf(stderr,"Error opening configuration file: %s \n",config_file);
	    return 1;
    	}
    fprintf(stderr,"parse configuration file: %s done!\n",config_file);
    return 0;
}

