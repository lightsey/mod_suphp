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


#define CORE_PRIVATE

#include "httpd.h"
#include "http_config.h"
#include "http_request.h"
#include "http_core.h"
#include "http_protocol.h"
#include "http_main.h"
#include "http_log.h"
#include "util_script.h"


#define SUPHP_CONFIG_MODE_SERVER 1
#define SUPHP_CONFIG_MODE_DIRECTORY 2

#define SUPHP_ENGINE_OFF 0
#define SUPHP_ENGINE_ON 1
#define SUPHP_ENGINE_UNDEFINED 2

#ifndef SUPHP_PATH_TO_SUPHP
#define SUPHP_PATH_TO_SUPHP "/usr/sbin/suphp"
#endif


/* Module declaration */
module MODULE_VAR_EXPORT suphp_module;


/* Configuration structure */
typedef struct {
    int engine; // Status of suPHP_Engine
    char *php_config;
    int cmode;  // Server of directory configuration?
#ifdef SUPHP_USE_USERGROUP
    char *target_user;
    char *target_group;
#endif
    table *handlers;
} suphp_conf;


/* Configuration mergers/creators */

static void *suphp_create_dir_config(pool *p, char *dir) {
    suphp_conf *cfg = (suphp_conf *) ap_pcalloc(p, sizeof(suphp_conf));
    cfg->php_config = NULL;
    cfg->engine = SUPHP_ENGINE_UNDEFINED;
    cfg->cmode = SUPHP_CONFIG_MODE_DIRECTORY;
    
#ifdef SUPHP_USE_USERGRPUP
    cfg->target_user = NULL;
    cfg->target_group = NULL;
#endif

    /* Create table with 0 initial elements */
    /* This size may be increased for performance reasons */
    cfg->handlers = ap_make_table(p, 0);
    
    return (void *) cfg;
}

static void *suphp_merge_dir_config(pool *p, void *base, void *overrides) {
    suphp_conf *parent = (suphp_conf *) base;
    suphp_conf *child = (suphp_conf *) overrides;
    suphp_conf *merged = (suphp_conf *) ap_pcalloc(p, sizeof(suphp_conf));

    merged->cmode = SUPHP_CONFIG_MODE_DIRECTORY;
    
    if (child->php_config)
	merged->php_config = ap_pstrdup(p, child->php_config);
    else if (parent->php_config)
	merged->php_config = ap_pstrdup(p, parent->php_config);
    else 
	merged->php_config = NULL;
    
    if (child->engine != SUPHP_ENGINE_UNDEFINED)
	merged->engine = child->engine;
    else
	merged->engine = parent->engine;

#ifdef SUPHP_USE_USERGROUP
    if (child->target_user)
	merged->target_user = ap_pstrdup(p, child->target_user);
    else if (parent->target_user)
	merged->target_user = ap_pstrdup(p, parent->target_user);
    else
	merged->target_user = NULL;

    if (child->target_group)
	merged->target_group = ap_pstrdup(p, child->target_group);
    else if (parent->target_group)
	merged->target_group = ap_pstrdup(p, parent->target_group);
    else
	merged->target_group = NULL;
#endif
    
    merged->handlers = ap_overlay_tables(p, child->handlers, parent->handlers);

    return (void *) merged;
}


static void *suphp_create_server_config(pool *p, server_rec *s) {
    suphp_conf *cfg = (suphp_conf *) ap_pcalloc(p, sizeof(suphp_conf));
    
    cfg->engine = SUPHP_ENGINE_UNDEFINED;
    cfg->cmode = SUPHP_CONFIG_MODE_SERVER;
    
#ifdef SUPHP_USE_USERGRPUP
    cfg->target_user = NULL;
    cfg->target_group = NULL;
#endif

    return (void *) cfg;
}


static void *suphp_merge_server_config(pool *p, void *base, void *overrides) {
    suphp_conf *parent = (suphp_conf *) base;
    suphp_conf *child = (suphp_conf *) overrides;
    suphp_conf *merged = (suphp_conf *) ap_pcalloc(p, sizeof(suphp_conf));
    
    if (child->engine != SUPHP_ENGINE_UNDEFINED)
	merged->engine = child->engine;
    else
	merged->engine = parent->engine;

#ifdef SUPHP_USE_USERGROUP
    if (child->target_user)
	merged->target_user = ap_pstrdup(p, child->target_user);
    else if (parent->target_user)
	merged->target_user = ap_pstrdup(p, parent->target_user);
    else
	merged->target_user = NULL;

    if (child->target_group)
	merged->target_group = ap_pstrdup(p, child->target_group);
    else if (parent->target_group)
	merged->target_group = ap_pstrdup(p, parent->target_group);
    else
	merged->target_group = NULL;
#endif

    return (void *) merged;
}


/* Command handlers */

static const char *suphp_handle_cmd_engine(cmd_parms *cmd, void *mconfig,
					   int flag) {
    suphp_conf *cfg;

    if (mconfig)
	cfg = (suphp_conf *) mconfig;
    else
	cfg = (suphp_conf *) ap_get_module_config(cmd->server->module_config,
						  &suphp_module);
    
    if (flag)
	cfg->engine = SUPHP_ENGINE_ON;
    else
	cfg->engine = SUPHP_ENGINE_OFF;

    return NULL;
}


static const char *suphp_handle_cmd_config(cmd_parms *cmd, void *mconfig,
					   const char *arg) {
    suphp_conf *cfg = (suphp_conf *) mconfig;
    
    cfg->php_config = ap_pstrdup(cmd->pool, arg);
    
    return NULL;
}


#ifdef SUPHP_USE_USERGROUP
static const char *suphp_handle_cmd_user_group(cmd_parms *cmd, void *mconfig,
					       const char *arg1, 
					       const char *arg2) {
    suphp_conf *cfg;

    if (mconfig)
	cfg = (suphp_conf *) mconfig;
    else
	cfg = ap_get_module_config(cmd->server->module_config, &suphp_module);
    
    cfg->target_user = ap_pstrdup(cmd->pool, arg1);
    cfg->target_group = ap_pstrdup(cmd->pool, arg2);

    return NULL;
}
#endif


static const char *suphp_handle_cmd_add_handler(cmd_parms *cmd, void *mconfig,
						const char *arg) {
    suphp_conf *cfg = (suphp_conf *) mconfig;

    // Mark active handlers with '1'
    ap_table_set(cfg->handlers, arg, "1");

    return NULL;
}

static const char *suphp_handle_cmd_remove_handler(cmd_parms *cmd, 
						   void *mconfig, 
						   const char *arg) {
    suphp_conf *cfg = (suphp_conf *) mconfig;
    
    // Mark deactivated handlers with '0'
    ap_table_set(cfg->handlers, arg, "0");

    return NULL;
}


/* Command table */

static const command_rec suphp_cmds[] = {
    {"suPHP_Engine", suphp_handle_cmd_engine, NULL, RSRC_CONF|ACCESS_CONF,
     FLAG, "Whether suPHP is on or off, default is off"},
    {"suPHP_ConfigPath", suphp_handle_cmd_config, NULL, OR_OPTIONS, TAKE1, 
     "Where the php.ini resides, default is the PHP default"},
#ifdef SUPHP_USE_USERGROUP
    {"suPHP_UserGroup", suphp_handle_cmd_user_group, NULL, 
     RSRC_CONF|ACCESS_CONF, TAKE2, "User and group scripts shall be run as"},
#endif 
    {"suPHP_AddHandler", suphp_handle_cmd_add_handler, NULL, ACCESS_CONF,
     ITERATE, "Tells mod_suphp to handle these MIME-types"},
    {"suphp_RemoveHandler", suphp_handle_cmd_remove_handler, NULL, ACCESS_CONF,
     ITERATE, "Tells mod_suphp not to handle these MIME-types"},
    {NULL}
};


/* Helper function which is called when spawning child process */

int suphp_child(void *rp, child_info *cinfo) {
    request_rec *r = (request_rec *) rp;
    core_dir_config *core_conf;
    pool *p = r->main ? r->main->pool : r->pool;
     
    char **argv, **env;

    core_conf = (core_dir_config *) ap_get_module_config(
	r->per_dir_config, &core_module);
    
    /* We want to log output written to stderr */
    ap_error_log2stderr(r->server);

    /* prepare argv for new process */
    
    argv = ap_palloc(p, 2 * sizeof(char *));
    argv[0] = SUPHP_PATH_TO_SUPHP;
    argv[1] = NULL;

    /* prepare environment */

    env = ap_create_environment(p, r->subprocess_env);

    /* We cannot use ap_call_exec because of the interference with suExec */
    /* So we do everything ourselves */
 
    /* Set resource limits from core config */
   
#ifdef RLIMIT_CPU
    if (core_conf->limit_cpu != NULL) {
	if ((setrlimit(RLIMIT_CPU, core_conf->limit_cpu)) != 0) {
	    ap_log_error(APLOG_MARK, APLOG_ERR, r->server, 
			 "setrlimit: failed to set CPU usage limit");
	}
    }
#endif /* RLIMIT_CPU */
#ifdef RLIMIT_NPROC
    if (core_conf->limit_nproc != NULL) {
	if ((setrlimit(RLIMIT_NPROC, core_conf->limit_nproc)) != 0) {
	    ap_log_error(APLOG_MARK, APLOG_ERR, r->server, 
			 "setrlimit: failed to set process limit");
	}
    }
#endif /* RLIMIT_NPROC */
#ifdef RLIMIT_AS
    if (core_conf->limit_mem != NULL) {
	if ((setrlimit(RLIMIT_AS, core_conf->limit_mem)) != 0) {
	    ap_log_error(APLOG_MARK, APLOG_ERR, r->server, 
			 "setrlimit: failed to set memory limit");
	}
    }
#endif /* RLIMIT_VMEM */
#ifdef RLIMIT_DATA
    if (core_conf->limit_mem != NULL) {
	if ((setrlimit(RLIMIT_DATA, core_conf->limit_mem)) != 0) {
	    ap_log_error(APLOG_MARK, APLOG_ERR, r->server, 
			 "setrlimit: failed to set memory limit");
	}
    }
#endif /* RLIMIT_VMEM */
#ifdef RLIMIT_VMEM
    if (core_conf->limit_mem != NULL) {
	if ((setrlimit(RLIMIT_VMEM, core_conf->limit_mem)) != 0) {
	    ap_log_error(APLOG_MARK, APLOG_ERR, r->server, 
			 "setrlimit: failed to set memory limit");
	}
    }
#endif /* RLIMIT_VMEM */

    /* mandatory cleanup before execution */
    ap_cleanup_for_exec();
    
    execve(SUPHP_PATH_TO_SUPHP, argv, env);

    /* We are still here? Okay - exec failed */
    ap_log_error(APLOG_MARK, APLOG_ERR, NULL, "exec of %s failed",
		 SUPHP_PATH_TO_SUPHP);
    exit(0);
    /* NOT REACHED */
    return (0);
}


/* Handlers */

static int suphp_handler(request_rec *r) {
    suphp_conf *sconf;
    suphp_conf *dconf;
    
    struct stat finfo;
    
    int rv;
    
    char *auth_user = NULL;
    char *auth_pass = NULL;

    pool *p;

    BUFF *script_in, *script_out, *script_err;

    sconf = ap_get_module_config(r->server->module_config, &suphp_module);
    dconf = ap_get_module_config(r->per_dir_config, &suphp_module);

    p = r->main ? r->main->pool : r->pool;

    /* only handle request if mod_suphp is active for this handler */
    /* check only first byte of value (second has to be \0) */
    if ((ap_table_get(dconf->handlers, r->handler) == NULL)
	|| (*(ap_table_get(dconf->handlers, r->handler)) == '0'))
	return DECLINED;

    /* check if suPHP is enabled for this request */

    if (((sconf->engine != SUPHP_ENGINE_ON)
	 && (dconf->engine != SUPHP_ENGINE_ON))
	|| ((sconf->engine == SUPHP_ENGINE_ON)
	    && (dconf->engine == SUPHP_ENGINE_OFF)))
	return DECLINED;

    /* check if file is existing and accessible */
    
    rv = stat(ap_pstrdup(p, r->filename), &finfo);
    if (rv == 0) {
	; /* do nothing */
    } else if (errno == EACCES) {
	ap_log_rerror(APLOG_MARK, APLOG_ERR, r, "access to %s denied",
		      r->filename);
	return HTTP_FORBIDDEN;
    } else if (errno == ENOENT || errno == ENOTDIR) {
	ap_log_rerror(APLOG_MARK, APLOG_ERR, r, "File does not exist: %s",
		      r->filename);
	return HTTP_NOT_FOUND;
    } else {
	ap_log_rerror(APLOG_MARK, APLOG_ERR, r, "could not get fileinfo: %s",
		      r->filename);
	return HTTP_NOT_FOUND;
    }

#ifdef SUPHP_USE_USERGROUP
    if ((sconf->target_user == NULL || sconf->target_group == NULL)
	&& (dconf->target_user == NULL || dconf->target_group == NULL)) {
	ap_log_rerror(APLOG_MARK, APLOG_ERR, r, 
		      "No user or group set - set suPHP_UserGroup");
	return HTTP_INTERNAL_SERVER_ERROR;
    }
#endif /* SUPHP_USE_USERGROUP */


    /* prepare environment for new process */
    
    ap_add_common_vars(r);
    ap_add_cgi_vars(r);

    ap_table_unset(r->subprocess_env, "SUPHP_PHP_CONFIG");
    ap_table_unset(r->subprocess_env, "SUHP_AUTH_USER");
    ap_table_unset(r->subprocess_env, "SUPHP_AUTH_PW");
    
#ifdef SUPHP_USE_USERGROUP
    ap_table_unset(r->subprocess_env, "SUPHP_USER");
    ap_table_unset(r->subprocess_env, "SUPHP_GROUP");
#endif /* SUPHP_USE_USERGROUP */

    if (dconf->php_config) {
	ap_table_set(r->subprocess_env, "SUPHP_PHP_CONFIG", dconf->php_config);
    }

    ap_table_set(r->subprocess_env, "SUPHP_HANDLER", r->handler);

    if (r->headers_in) {
	const char *auth;
	auth = ap_table_get(r->headers_in, "Authorization");
	if (auth && auth[0] != 0 && strncmp(auth, "Basic ", 6) == 0) {
	    char *user;
	    char *pass;
	    user = ap_pbase64decode(p, auth + 6);
	    if (user) {
		pass = strchr(user, ':');
		if (pass) {
		    *pass++ = '\0';
		    auth_user = ap_pstrdup(p, user);
		    auth_pass = ap_pstrdup(p, pass);
		}
	    }
	}
    }
    
    if (auth_user && auth_pass) {
	ap_table_setn(r->subprocess_env, "SUPHP_AUTH_USER", auth_user);
	ap_table_setn(r->subprocess_env, "SUPHP_AUTH_PW", auth_pass);
    }

#ifdef SUPHP_USE_USERGROUP
    if (dconf->target_user) {
	ap_table_set(r->subprocess_env, "SUPHP_USER", dconf->target_user);
    } else {
	ap_table_set(r->subprocess_env, "SUPHP_USER", sconf->target_user);
    }

    if (dconf->target_group) {
	ap_table_set(r->subprocess_env, "SUPHP_GROUP", dconf->target_group);
    } else {
	ap_table_set(r->subprocess_env, "SUPHP_GROUP", sconf->target_group);
    }
#endif /* SUPHP_USE_USERGROUP */
    
    /* Fork child process */
    
    if (!ap_bspawn_child(p, suphp_child, (void *) r, kill_after_timeout,
			 &script_in, &script_out, &script_err)) {
	ap_log_rerror(APLOG_MARK, APLOG_ERR, r,
		      "couldn't spawn child process for: %s", r->filename);
	return HTTP_INTERNAL_SERVER_ERROR;
    }
    
    /* Transfer request body to script */
    
    if ((rv = ap_setup_client_block(r, REQUEST_CHUNKED_ERROR)) != OK) {
	/* Call failed, return status */
	return rv;
    }
    
    if (ap_should_client_block(r)) {
	char buffer[HUGE_STRING_LEN];
	int len_read;

	ap_hard_timeout("reading request body", r);

	while ((len_read = ap_get_client_block(r, buffer, HUGE_STRING_LEN)) 
	       > 0) {
	    ap_reset_timeout(r);
	    if (ap_bwrite(script_in, buffer, len_read) < len_read) {
		/* silly script stopped reading, soak up remaining message */
		while (ap_get_client_block(r, buffer, HUGE_STRING_LEN) > 0) {
		    /* dump it */
		}
		break;
	    }
	}
	
	ap_bflush(script_in);
	ap_kill_timeout(r);
    }

    ap_bclose(script_in);
    
    /* Transfer output from script to client */

    if (script_out) {
	const char *location;
	char hbuffer[MAX_STRING_LEN];
	char buffer[HUGE_STRING_LEN];
	
	if (rv = ap_scan_script_header_err_buff(r, script_out, hbuffer)) {
	    return HTTP_INTERNAL_SERVER_ERROR;
	}
	
	location = ap_table_get(r->headers_out, "Location");
	if (location && r->status == 200) {
	    /* Soak up all the script output */
	    ap_hard_timeout("reading from script", r);
	    while (ap_bgets(buffer, HUGE_STRING_LEN, script_out) > 0) {
		continue;
	    }
	    ap_kill_timeout(r);
	    ap_bclose(script_out);
	    ap_bclose(script_err);
	
	    if (location[0] == '/') { 
		/* Redirect has always GET method */
		r->method = ap_pstrdup(p, "GET");
		r->method_number = M_GET;
		
		/* Remove Content-Length - redirect should not read  *
		 * request body                                      */
		ap_table_unset(r->headers_in, "Content-Length");
		
		/* Do the redirect */
		ap_internal_redirect_handler(location, r);
		return OK;
	    } else {
		/* Script did not set status 302 - so it does not want *
		 * to send its own body. Simply set redirect status    */
		return REDIRECT;
	    }
	}
	    
	/* Output headers and body */

	ap_send_http_header(r);
	if (!r->header_only) {
	    ap_send_fb(script_out, r);
	}
	ap_bclose(script_out);
	/* Errors have already been logged by child */
	ap_bclose(script_err);
    }
    
    return OK;
}


/* Handlers table */

static handler_rec suphp_handlers[] = {
    {"*", suphp_handler},
    {NULL}
};

/* Module definition */

module MODULE_VAR_EXPORT suphp_module = {
    STANDARD_MODULE_STUFF,
    NULL, /* initializer */
    suphp_create_dir_config,     /* create directory config */
    suphp_merge_dir_config,      /* merge directory config */
    suphp_create_server_config,  /* create server config */
    suphp_merge_server_config,   /* merge server config */
    suphp_cmds,                  /* command table */
    suphp_handlers,              /* content handlers */
    NULL,                        /* URI-to-filename translation */
    NULL,                        /* check/validate user_id */
    NULL,                        /* check user_id is valid *here* */
    NULL,                        /* check access by host address */
    NULL,                        /* MIME type checker/setter */
    NULL,                        /* fixups */
    NULL,                        /* logger */
    NULL,                        /* header parser */
    NULL,                        /* process initialaization */
    NULL,                        /* process exit/cleanup */
    NULL                         /* post read_request handling */
};
