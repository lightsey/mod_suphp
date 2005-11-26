/*
    suPHP - (c)2002-2005 Sebastian Marsching <sebastian@marsching.com>
    
    This file is part of suPHP.

    suPHP is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    suPHP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with suPHP; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "apr.h"
#include "apr_strings.h"
#include "apr_thread_proc.h"
#include "apr_buckets.h"

#define CORE_PRIVATE

#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"

#include "util_script.h"
#include "util_filter.h"


module AP_MODULE_DECLARE_DATA suphp_module;


/*********************
  Auxiliary functions
 *********************/

static int suphp_bucket_read(apr_bucket *b, char *buf, int len)
{
    const char *dst_end = buf + len - 1;
    char * dst = buf;
    apr_status_t rv;
    const char *bucket_data;
    apr_size_t bucket_data_len;
    const char *src;
    const char *src_end;
    int count = 0;
    
    if (APR_BUCKET_IS_EOS(b))
        return -1;
       
    rv = apr_bucket_read(b, &bucket_data, &bucket_data_len, APR_BLOCK_READ);
    if (!APR_STATUS_IS_SUCCESS(rv) || (bucket_data_len == 0))
    {
        return 0;
    }
    src = bucket_data;
    src_end = bucket_data + bucket_data_len;
    while ((src < src_end) && (dst < dst_end))
    {
	*dst = *src;
     dst++;
     src++;
     count++;
    }
    *dst = 0;
    return count;
}


static void suphp_log_script_err(request_rec *r, apr_file_t *script_err)
{
    char argsbuffer[HUGE_STRING_LEN];
    char *newline;

    while (apr_file_gets(argsbuffer, HUGE_STRING_LEN,
                         script_err) == APR_SUCCESS) {
        newline = strchr(argsbuffer, '\n');
        if (newline) {
            *newline = '\0';
        }
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "%s", argsbuffer);
    }
}


/**************************
  Configuration processing
 **************************/

#define SUPHP_CONFIG_MODE_SERVER 1
#define SUPHP_CONFIG_MODE_DIRECTORY 2

#define SUPHP_ENGINE_OFF 0
#define SUPHP_ENGINE_ON 1
#define SUPHP_ENGINE_UNDEFINED 2

#ifndef SUPHP_PATH_TO_SUPHP
#define SUPHP_PATH_TO_SUPHP "/usr/sbin/suphp"
#endif

typedef struct {
    int engine; // Status of suPHP_Engine
    char *php_config;
    int cmode;  // Server of directory configuration?
#ifdef SUPHP_USE_USERGROUP
    char *target_user;
    char *target_group;
#endif
    apr_table_t *handlers;
} suphp_conf;


static void *suphp_create_dir_config(apr_pool_t *p, char *dir)
{
    suphp_conf *cfg = (suphp_conf *) apr_pcalloc(p, sizeof(suphp_conf));
    
    cfg->php_config = NULL;
    cfg->engine = SUPHP_ENGINE_UNDEFINED;
    cfg->cmode = SUPHP_CONFIG_MODE_DIRECTORY;

#ifdef SUPHP_USE_USERGROUP
    cfg->target_user = NULL;
    cfg->target_group = NULL;
#endif
    
    /* Create table with 0 initial elements */
    /* This size may be increased for performance reasons */
    cfg->handlers = apr_table_make(p, 0);
    
    return (void *) cfg;
}


static void *suphp_merge_dir_config(apr_pool_t *p, void *base, 
                                    void *overrides)
{
    suphp_conf *parent = (suphp_conf *) base;
    suphp_conf *child = (suphp_conf *) overrides;
    suphp_conf *merged = (suphp_conf *) apr_pcalloc(p, sizeof(suphp_conf));
    
    merged->cmode = SUPHP_CONFIG_MODE_DIRECTORY;
    
    if (child->php_config)
        merged->php_config = apr_pstrdup(p, child->php_config);
    else if (parent->php_config)
        merged->php_config = apr_pstrdup(p, parent->php_config);
    else
        merged->php_config = NULL;
    
    if (child->engine != SUPHP_ENGINE_UNDEFINED)
        merged->engine = child->engine;
    else
        merged->engine = parent->engine;

#ifdef SUPHP_USE_USERGROUP
    if (child->target_user)
        merged->target_user = apr_pstrdup(p, child->target_user);
    else if (parent->target_user)
        merged->target_user = apr_pstrdup(p, parent->target_user);
    else
        merged->target_user = NULL;
        
    if (child->target_group)
        merged->target_group = apr_pstrdup(p, child->target_group);
    else if (parent->target_group)
        merged->target_group = apr_pstrdup(p, parent->target_group);
    else
        merged->target_group = NULL;
#endif
    
    merged->handlers = apr_table_overlay(p, child->handlers, parent->handlers);
    
    return (void *) merged;  
}


static void *suphp_create_server_config(apr_pool_t *p, server_rec *s)
{
    suphp_conf *cfg = (suphp_conf *) apr_pcalloc(p, sizeof(suphp_conf));
    
    cfg->engine = SUPHP_ENGINE_UNDEFINED;
    cfg->cmode = SUPHP_CONFIG_MODE_SERVER;
    
    return (void *) cfg;
}


static void *suphp_merge_server_config(apr_pool_t *p, void *base,
                                       void *overrides)
{
    suphp_conf *parent = (suphp_conf *) base;
    suphp_conf *child = (suphp_conf *) overrides;
    suphp_conf *merged = (suphp_conf *) apr_pcalloc(p, sizeof(suphp_conf));
    
    if (child->engine != SUPHP_ENGINE_UNDEFINED)
        merged->engine = child->engine;
    else
        merged->engine = parent->engine;

#ifdef SUPHP_USE_USERGROUP
    if (child->target_user)
        merged->target_user = apr_pstrdup(p, child->target_user);
    else if (parent->target_user)
        merged->target_user = apr_pstrdup(p, parent->target_user);
    else
        merged->target_user = NULL;
        
    if (child->target_group)
        merged->target_group = apr_pstrdup(p, child->target_group);
    else if (parent->target_group)
        merged->target_group = apr_pstrdup(p, parent->target_group);
    else
        merged->target_group = NULL;
#endif
    
    return (void*) merged;
}


/******************
  Command handlers
 ******************/

static const char *suphp_handle_cmd_engine(cmd_parms *cmd, void *mconfig,
                                           int flag)
{
    server_rec *s = cmd->server;
    suphp_conf *cfg;
    
    if (mconfig)
        cfg = (suphp_conf *) mconfig;
    else
        cfg = (suphp_conf *) ap_get_module_config(s->module_config, &suphp_module);
    
    if (flag)
        cfg->engine = SUPHP_ENGINE_ON;
    else
        cfg->engine = SUPHP_ENGINE_OFF;
    
    return NULL;
}


static const char *suphp_handle_cmd_config(cmd_parms *cmd, void *mconfig,
                                           const char *arg)
{
    server_rec *s = cmd->server;
    suphp_conf *cfg;
    
    if (mconfig)
        cfg = (suphp_conf *) mconfig;
    else
        cfg = (suphp_conf *) ap_get_module_config(s->module_config, &suphp_module);
    
    cfg->php_config = apr_pstrdup(cmd->pool, arg);
    
    return NULL;
}


#ifdef SUPHP_USE_USERGROUP
static const char *suphp_handle_cmd_user_group(cmd_parms *cmd, void *mconfig,
                                           const char *arg1, const char *arg2)
{
    suphp_conf *cfg = (suphp_conf *) mconfig;
    
    cfg->target_user = apr_pstrdup(cmd->pool, arg1);
    cfg->target_group = apr_pstrdup(cmd->pool, arg2);
    
    return NULL;
}
#endif


static const char *suphp_handle_cmd_add_handler(cmd_parms *cmd, void *mconfig,
					     const char *arg)
{
    suphp_conf *cfg = (suphp_conf *) mconfig;
    // Mark active handler with '1'
    apr_table_set(cfg->handlers, arg, "1");

    return NULL;
}


static const char *suphp_handle_cmd_remove_handler(cmd_parms *cmd, 
						   void *mconfig, 
						   const char *arg)
{
    suphp_conf *cfg = (suphp_conf *) mconfig;
    // Mark deactivated handler with '0'
    apr_table_set(cfg->handlers, arg, "0");

    return NULL;
}


static const command_rec suphp_cmds[] =
{
    AP_INIT_FLAG("suPHP_Engine", suphp_handle_cmd_engine, NULL, RSRC_CONF | ACCESS_CONF,
                 "Whether suPHP is on or off, default is off"),
    AP_INIT_TAKE1("suPHP_ConfigPath", suphp_handle_cmd_config, NULL, OR_OPTIONS,
                  "Wheres the php.ini resides, default is the PHP default"),
#ifdef SUPHP_USE_USERGROUP
    AP_INIT_TAKE2("suPHP_UserGroup", suphp_handle_cmd_user_group, NULL, RSRC_CONF | ACCESS_CONF,
                  "User and group scripts shall be run as"),
#endif
    AP_INIT_ITERATE("suPHP_AddHandler", suphp_handle_cmd_add_handler, NULL, ACCESS_CONF, "Tells mod_suphp to handle these MIME-types"),
    AP_INIT_ITERATE("suPHP_RemoveHandler", suphp_handle_cmd_remove_handler, NULL, ACCESS_CONF, "Tells mod_suphp not to handle these MIME-types"),
    {NULL}
};


/******************
  Hooks / handlers
 ******************/

static int suphp_handler(request_rec *r)
{
    apr_pool_t *p;
    suphp_conf *sconf;
    suphp_conf *dconf;
    core_dir_config *core_conf;
    
    apr_finfo_t finfo;
    
    apr_procattr_t *procattr;
    
    apr_proc_t *proc;
    
    char **argv;
    char **env;
    apr_status_t rv;
    int len = 0;
#if MAX_STRING_LEN < 1024
    char strbuf[1024];
#else
    char strbuf[MAX_STRING_LEN];
#endif
    int nph = 0;
    int eos_reached = 0;
    char *auth_user = NULL;
    char *auth_pass = NULL;
    
    apr_bucket_brigade *bb;
    apr_bucket *b;
    
    /* load configuration */
    
    p = r->main ? r->main->pool : r->pool;
    sconf = ap_get_module_config(r->server->module_config, &suphp_module);
    dconf = ap_get_module_config(r->per_dir_config, &suphp_module);
    core_conf = (core_dir_config *) ap_get_module_config(r->per_dir_config, &core_module);
    
    /* only handle request if mod_suphp is active for this handler */
    /* check only first byte of value (second has to be \0) */
    if ((apr_table_get(dconf->handlers, r->handler) == NULL)
	|| (*(apr_table_get(dconf->handlers, r->handler)) == '0'))
	return DECLINED;
    
    /* check if suPHP is enabled for this request */
    
    if (((sconf->engine != SUPHP_ENGINE_ON)
        && (dconf->engine != SUPHP_ENGINE_ON))
        || ((sconf->engine == SUPHP_ENGINE_ON)
        && (dconf->engine == SUPHP_ENGINE_OFF)))
        return DECLINED;
    
    /* check if file is existing and acessible */
    
    rv = apr_stat(&finfo, apr_pstrdup(p, r->filename), APR_FINFO_NORM, p);
    
    if (rv == APR_SUCCESS)
        ; /* do nothing */
    else if (rv == EACCES)
    {
        return HTTP_FORBIDDEN;
        ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r, "access to %s denied", r->filename);
    }
    else if (rv == ENOENT)
    {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "File does not exist: %s", r->filename);
        return HTTP_NOT_FOUND;
    }
    else
    {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r, "could not get fileinfo: %s", r->filename);
        return HTTP_NOT_FOUND;
    }
    
    if (!(r->finfo.protection & APR_UREAD))
    {
    	ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "Insufficient permissions: %s", r->filename);
    	return HTTP_FORBIDDEN;
    }
    
#ifdef SUPHP_USE_USERGROUP
    if ((sconf->target_user == NULL || sconf->target_group == NULL)
        && (dconf->target_user == NULL || dconf->target_group == NULL))
    {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, 
                      "No user or group set - set suPHP_UserGroup");
        return HTTP_INTERNAL_SERVER_ERROR;
    }
#endif
        
    /* prepare argv for new process */
    
    argv = apr_palloc(p, 2 * sizeof(char *));
    argv[0] = SUPHP_PATH_TO_SUPHP;
    argv[1] = NULL;
    
    /* prepare environment for new process */
    
    ap_add_common_vars(r);
    ap_add_cgi_vars(r);
    
    apr_table_unset(r->subprocess_env, "SUPHP_PHP_CONFIG");
    apr_table_unset(r->subprocess_env, "SUPHP_AUTH_USER");
    apr_table_unset(r->subprocess_env, "SUPHP_AUTH_PW");
    
#ifdef SUPHP_USE_USERGROUP
    apr_table_unset(r->subprocess_env, "SUPHP_USER");
    apr_table_unset(r->subprocess_env, "SUPHP_GROUP");
#endif
    
    if (dconf->php_config)
    {
        apr_table_setn(r->subprocess_env, "SUPHP_PHP_CONFIG", apr_pstrdup(p, dconf->php_config));
    }
    
    apr_table_setn(r->subprocess_env, "SUPHP_HANDLER", r->handler);
    
    if (r->headers_in)
    {
        const char *auth = NULL;
        auth = apr_table_get(r->headers_in, "Authorization");
        if (auth && auth[0] != 0 && strncmp(auth, "Basic ", 6) == 0)
        {
            char *user;
            char *pass;
            user = ap_pbase64decode(p, auth + 6);
            if (user)
            {
                pass = strchr(user, ':');
                if (pass)
                {
                    *pass++ = '\0';
                    auth_user = apr_pstrdup(r->pool, user);
                    auth_pass = apr_pstrdup(r->pool, pass);
                }
            }
        }
    }
    
    if (auth_user && auth_pass)
    {
        apr_table_setn(r->subprocess_env, "SUPHP_AUTH_USER", auth_user);
        apr_table_setn(r->subprocess_env, "SUPHP_AUTH_PW", auth_pass);
    }

#ifdef SUPHP_USE_USERGROUP
    if (dconf->target_user)
    {
        apr_table_setn(r->subprocess_env, "SUPHP_USER",
                       apr_pstrdup(r->pool, dconf->target_user));
    }
    else
    {
        apr_table_setn(r->subprocess_env, "SUPHP_USER",
                       apr_pstrdup(r->pool, sconf->target_user));
    }
    
    if (dconf->target_group)
    {
        apr_table_setn(r->subprocess_env, "SUPHP_GROUP",
                       apr_pstrdup(r->pool, dconf->target_group));
    }
    else
    {
        apr_table_setn(r->subprocess_env, "SUPHP_GROUP",
                       apr_pstrdup(r->pool, sconf->target_group));
    }
#endif
    
    env = ap_create_environment(p, r->subprocess_env);
        
    /* set attributes for new process */
    
    if (((rv = apr_procattr_create(&procattr, p)) != APR_SUCCESS)
        || ((rv = apr_procattr_io_set(procattr, APR_CHILD_BLOCK, APR_CHILD_BLOCK, APR_CHILD_BLOCK)) != APR_SUCCESS)
        || ((rv = apr_procattr_dir_set(procattr, ap_make_dirstr_parent(r->pool, r->filename))) != APR_SUCCESS)
    
    /* set resource limits */

#ifdef RLIMIT_CPU
        || ((rv = apr_procattr_limit_set(procattr, APR_LIMIT_CPU, core_conf->limit_cpu)) != APR_SUCCESS)
#endif
#if defined(RLIMIT_DATA) || defined(RLIMIT_VMEM) || defined(RLIMIT_AS)
        || ((rv = apr_procattr_limit_set(procattr, APR_LIMIT_MEM, core_conf->limit_mem)) != APR_SUCCESS)
#endif
#ifdef RLIMIT_NPROC
        || ((apr_procattr_limit_set(procattr, APR_LIMIT_NPROC, core_conf->limit_nproc)) != APR_SUCCESS)
#endif

        || ((apr_procattr_cmdtype_set(procattr, APR_PROGRAM)) != APR_SUCCESS)
        || ((apr_procattr_error_check_set(procattr, 1)) != APR_SUCCESS)
        || ((apr_procattr_detach_set(procattr, 0)) != APR_SUCCESS))
    {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                      "couldn't set child process attributes: %s", r->filename);
        return HTTP_INTERNAL_SERVER_ERROR;
    }
    
    /* create new process */
    

    proc = apr_pcalloc(p, sizeof(*proc));
    rv = apr_proc_create(proc, SUPHP_PATH_TO_SUPHP, (const char *const *)argv, (const char *const *)env, procattr, p);
    if (rv != APR_SUCCESS)
    {
       ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                     "couldn't create child process: %s for %s", SUPHP_PATH_TO_SUPHP, r->filename);
       return HTTP_INTERNAL_SERVER_ERROR;
    }
    apr_pool_note_subprocess(p, proc, APR_KILL_AFTER_TIMEOUT);

    if (!proc->out)
        return APR_EBADF;
    apr_file_pipe_timeout_set(proc->out, r->server->timeout);
    
    if (!proc->in)
        return APR_EBADF;
    apr_file_pipe_timeout_set(proc->in, r->server->timeout);
    
    if (!proc->err)
        return APR_EBADF;
    apr_file_pipe_timeout_set(proc->err, r->server->timeout);
    
    /* send request body to script */
    
    bb = apr_brigade_create(r->pool, r->connection->bucket_alloc);
    do
    {
        apr_bucket *bucket;
        rv = ap_get_brigade(r->input_filters, bb, AP_MODE_READBYTES, APR_BLOCK_READ, HUGE_STRING_LEN);
        
        if (rv != APR_SUCCESS)
        {
            return rv;
        }
        
        APR_BRIGADE_FOREACH(bucket, bb)
        {
            const char *data;
            apr_size_t len;
            int child_stopped_reading = 0;
            
            if (APR_BUCKET_IS_EOS(bucket))
            {
                eos_reached = 1;
                break;
            }
            
            if (APR_BUCKET_IS_FLUSH(bucket) || child_stopped_reading)
            {
                continue;
            }
            
            apr_bucket_read(bucket, &data, &len, APR_BLOCK_READ);
            
            rv = apr_file_write_full(proc->in, data, len, NULL);
            if (rv != APR_SUCCESS)
            {
                child_stopped_reading = 1;
            }
        }
        apr_brigade_cleanup(bb);
    }
    while (!eos_reached);
    
    apr_file_flush(proc->in);
    apr_file_close(proc->in);
    
    /* get output from script and check if non-parsed headers are used */
    
    b = apr_bucket_pipe_create(proc->out, r->connection->bucket_alloc);
    APR_BRIGADE_INSERT_TAIL(bb, b);
    
    len = 8;
    if ((suphp_bucket_read(b, strbuf, len) == len)
        && !(strcmp(strbuf, "HTTP/1.0") && strcmp(strbuf, "HTTP/1.1")))
    {
        nph = 1;
    }
    
    b = apr_bucket_eos_create(r->connection->bucket_alloc);
    APR_BRIGADE_INSERT_TAIL(bb, b);
    
    if (proc->out && !nph)
    {
        /* normal cgi headers, so we have to create the real headers by hand */
        
        int ret;
        const char *location;
        
	ret = ap_scan_script_header_err_brigade(r, bb, strbuf);
	if (ret == HTTP_NOT_MODIFIED)
	{
	    return ret;
	}
        else if (ret != APR_SUCCESS)
        {
            suphp_log_script_err(r, proc->err);
            
            /* ap_scan_script_header_err_brigade does logging itself,
               so simply return                                       */
               
            return HTTP_INTERNAL_SERVER_ERROR;
        }
        
        location = apr_table_get(r->headers_out, "Location");
        if (location && location[0] == '/' && r->status == 200)
        {
            /* empty brigade (script output) and modify headers */
            
            const char *buf;
            apr_size_t blen;
            APR_BRIGADE_FOREACH(b, bb)
            {
                if (APR_BUCKET_IS_EOS(b))
                    break;
                if (apr_bucket_read(b, &buf, &blen, APR_BLOCK_READ) != APR_SUCCESS)
                    break;
            }
            apr_brigade_destroy(bb);
            suphp_log_script_err(r, proc->err);
            r->method = apr_pstrdup(r->pool, "GET");
            r->method_number = M_GET;
            apr_table_unset(r->headers_in, "Content-Length");
            
            ap_internal_redirect_handler(location, r);
            return OK;
        }
        else if (location && r->status == 200)
        {
            /* empty brigade (script output) */
            const char *buf;
            apr_size_t blen;
            APR_BRIGADE_FOREACH(b, bb)
            {
                if (APR_BUCKET_IS_EOS(b))
                    break;
                if (apr_bucket_read(b, &buf, &blen, APR_BLOCK_READ) != APR_SUCCESS)
                    break;
            }
            apr_brigade_destroy(bb);
            return HTTP_MOVED_TEMPORARILY;
        }
        
        /* send output to browser (through filters) */
        
        rv = ap_pass_brigade(r->output_filters, bb);
        
        /* write errors to logfile */
        
        if (rv == APR_SUCCESS && !r->connection->aborted)
            suphp_log_script_err(r, proc->err);
        
        apr_file_close(proc->err);
    }
    
    if (proc->out && nph)
    {
        /* use non-parsed headers (direct output) */
        
        struct ap_filter_t *cur;
        
        /* get rid of output filters */
        
        cur = r->proto_output_filters;
        while (cur && cur->frec->ftype < AP_FTYPE_CONNECTION)
        {
            cur = cur->next;
        }
        r->output_filters = r->proto_output_filters = cur;
        
        /* send output to browser (directly) */
        
        rv = ap_pass_brigade(r->output_filters, bb);
        
        /* log errors */
        if (rv == APR_SUCCESS && !r->connection->aborted)
            suphp_log_script_err(r, proc->err);
            
        apr_file_close(proc->err);
    }

    return OK;
}

static void suphp_register_hooks(apr_pool_t *p)
{
    ap_hook_handler(suphp_handler, NULL, NULL, APR_HOOK_MIDDLE);
}


/********************
  Module declaration
 ********************/
 
module AP_MODULE_DECLARE_DATA suphp_module =
{
    STANDARD20_MODULE_STUFF,
    suphp_create_dir_config,
    suphp_merge_dir_config,
    suphp_create_server_config,
    suphp_merge_server_config,
    suphp_cmds,
    suphp_register_hooks
};
