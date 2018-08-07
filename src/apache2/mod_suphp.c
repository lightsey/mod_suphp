/*
  suPHP - (c)2002-2013 Sebastian Marsching <sebastian@marsching.com>
          (c)2018 John Lightsey <john@nixnuts.net>

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
  along with suPHP; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "apr.h"
#include "apr_buckets.h"
#include "apr_poll.h"
#include "apr_strings.h"
#include "apr_thread_proc.h"

#define CORE_PRIVATE

#include "httpd.h"

#include "http_config.h"
#include "http_core.h"
#include "http_log.h"

#if AP_SERVER_MAJORVERSION_NUMBER >= 2 && AP_SERVER_MINORVERSION_NUMBER >= 4
#include "http_request.h"
#endif

#include "util_filter.h"
#include "util_script.h"

/* needed for get_suexec_identity hook */
#include "unixd.h"

module AP_MODULE_DECLARE_DATA suphp_module;

/*********************
  Auxiliary functions
 *********************/

static apr_status_t suphp_log_script_err(request_rec *r,
                                         apr_file_t *script_err) {
  char argsbuffer[HUGE_STRING_LEN];
  char *newline;
  apr_status_t rv;

  while ((rv = apr_file_gets(argsbuffer, HUGE_STRING_LEN, script_err)) ==
         APR_SUCCESS) {
    newline = strchr(argsbuffer, '\n');
    if (newline) {
      *newline = '\0';
    }
    ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "%s", argsbuffer);
  }

  return rv;
}

char *suphp_brigade_read(apr_pool_t *p, apr_bucket_brigade *bb, int bytes) {
  char *target_buf;
  char *next_byte;
  char *last_byte;
  apr_bucket *b;

  if (bytes == 0) {
    return NULL;
  }

  target_buf = (char *)apr_palloc(p, bytes + 1);
  next_byte = target_buf;
  last_byte = target_buf + bytes;

  for (b = APR_BRIGADE_FIRST(bb); b != APR_BRIGADE_SENTINEL(bb);
       b = APR_BUCKET_NEXT(b)) {
    const char *buf;
    apr_size_t size;
    apr_size_t i;
    if (apr_bucket_read(b, &buf, &size, APR_BLOCK_READ) == APR_SUCCESS) {
      for (i = 0; i < size; i++) {
        *next_byte = *buf;
        next_byte++;
        buf++;
        if (next_byte == last_byte) {
          *next_byte = 0;
          return target_buf;
        }
      }
    }
  }
  next_byte = 0;
  return target_buf;
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
  int engine;  // Status of suPHP_Engine
  char *php_config;
  int cmode;  // Server of directory configuration?
  char *target_user;
  char *target_group;
  apr_table_t *handlers;
  char *php_path;
} suphp_conf;

static void *suphp_create_dir_config(apr_pool_t *p, char *dir) {
  suphp_conf *cfg = (suphp_conf *)apr_pcalloc(p, sizeof(suphp_conf));

  cfg->php_config = NULL;
  cfg->engine = SUPHP_ENGINE_UNDEFINED;
  cfg->php_path = NULL;
  cfg->cmode = SUPHP_CONFIG_MODE_DIRECTORY;
  cfg->target_user = NULL;
  cfg->target_group = NULL;

  /* Create table with 0 initial elements */
  /* This size may be increased for performance reasons */
  cfg->handlers = apr_table_make(p, 0);

  return (void *)cfg;
}

static void *suphp_merge_dir_config(apr_pool_t *p, void *base,
                                    void *overrides) {
  suphp_conf *parent = (suphp_conf *)base;
  suphp_conf *child = (suphp_conf *)overrides;
  suphp_conf *merged = (suphp_conf *)apr_pcalloc(p, sizeof(suphp_conf));

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

  merged->handlers = apr_table_overlay(p, child->handlers, parent->handlers);

  return (void *)merged;
}

static void *suphp_create_server_config(apr_pool_t *p, server_rec *s) {
  suphp_conf *cfg = (suphp_conf *)apr_pcalloc(p, sizeof(suphp_conf));

  cfg->engine = SUPHP_ENGINE_UNDEFINED;
  cfg->php_path = NULL;
  cfg->cmode = SUPHP_CONFIG_MODE_SERVER;

  /* Create table with 0 initial elements */
  /* This size may be increased for performance reasons */
  cfg->handlers = apr_table_make(p, 0);

  return (void *)cfg;
}

static void *suphp_merge_server_config(apr_pool_t *p, void *base,
                                       void *overrides) {
  suphp_conf *parent = (suphp_conf *)base;
  suphp_conf *child = (suphp_conf *)overrides;
  suphp_conf *merged = (suphp_conf *)apr_pcalloc(p, sizeof(suphp_conf));

  if (child->engine != SUPHP_ENGINE_UNDEFINED)
    merged->engine = child->engine;
  else
    merged->engine = parent->engine;

  if (child->php_path != NULL)
    merged->php_path = apr_pstrdup(p, child->php_path);
  else
    merged->php_path = apr_pstrdup(p, parent->php_path);

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

  merged->handlers = apr_table_overlay(p, child->handlers, parent->handlers);

  return (void *)merged;
}

/******************
  Command handlers
 ******************/

static const char *suphp_handle_cmd_engine(cmd_parms *cmd, void *mconfig,
                                           int flag) {
  server_rec *s = cmd->server;
  suphp_conf *cfg;

  if (mconfig)
    cfg = (suphp_conf *)mconfig;
  else
    cfg = (suphp_conf *)ap_get_module_config(s->module_config, &suphp_module);

  if (flag)
    cfg->engine = SUPHP_ENGINE_ON;
  else
    cfg->engine = SUPHP_ENGINE_OFF;

  return NULL;
}

static const char *suphp_handle_cmd_config(cmd_parms *cmd, void *mconfig,
                                           const char *arg) {
  server_rec *s = cmd->server;
  suphp_conf *cfg;

  if (mconfig)
    cfg = (suphp_conf *)mconfig;
  else
    cfg = (suphp_conf *)ap_get_module_config(s->module_config, &suphp_module);

  cfg->php_config = apr_pstrdup(cmd->pool, arg);

  return NULL;
}

static const char *suphp_handle_cmd_user_group(cmd_parms *cmd, void *mconfig,
                                               const char *arg1,
                                               const char *arg2) {
  suphp_conf *cfg = (suphp_conf *)mconfig;

  cfg->target_user = apr_pstrdup(cmd->pool, arg1);
  cfg->target_group = apr_pstrdup(cmd->pool, arg2);

  return NULL;
}

static const char *suphp_handle_cmd_add_handler(cmd_parms *cmd, void *mconfig,
                                                const char *arg) {
  suphp_conf *cfg;
  if (mconfig)
    cfg = (suphp_conf *)mconfig;
  else
    cfg = (suphp_conf *)ap_get_module_config(cmd->server->module_config,
                                             &suphp_module);

  // Mark active handler with '1'
  apr_table_set(cfg->handlers, arg, "1");

  return NULL;
}

static const char *suphp_handle_cmd_remove_handler(cmd_parms *cmd,
                                                   void *mconfig,
                                                   const char *arg) {
  suphp_conf *cfg;
  if (mconfig)
    cfg = (suphp_conf *)mconfig;
  else
    cfg = (suphp_conf *)ap_get_module_config(cmd->server->module_config,
                                             &suphp_module);

  // Mark deactivated handler with '0'
  apr_table_set(cfg->handlers, arg, "0");

  return NULL;
}

static const char *suphp_handle_cmd_phppath(cmd_parms *cmd, void *mconfig,
                                            const char *arg) {
  server_rec *s = cmd->server;
  suphp_conf *cfg;

  cfg = (suphp_conf *)ap_get_module_config(s->module_config, &suphp_module);

  cfg->php_path = apr_pstrdup(cmd->pool, arg);

  return NULL;
}

static const command_rec suphp_cmds[] = {
    AP_INIT_FLAG("suPHP_Engine", suphp_handle_cmd_engine, NULL,
                 RSRC_CONF | ACCESS_CONF,
                 "Whether suPHP is on or off, default is off"),
    AP_INIT_TAKE1("suPHP_ConfigPath", suphp_handle_cmd_config, NULL, OR_OPTIONS,
                  "Wheres the php.ini resides, default is the PHP default"),
    AP_INIT_TAKE2("suPHP_UserGroup", suphp_handle_cmd_user_group, NULL,
                  RSRC_CONF | ACCESS_CONF,
                  "User and group scripts shall be run as"),
    AP_INIT_ITERATE("suPHP_AddHandler", suphp_handle_cmd_add_handler, NULL,
                    RSRC_CONF | ACCESS_CONF,
                    "Tells mod_suphp to handle these MIME-types"),
    AP_INIT_ITERATE("suPHP_RemoveHandler", suphp_handle_cmd_remove_handler,
                    NULL, RSRC_CONF | ACCESS_CONF,
                    "Tells mod_suphp not to handle these MIME-types"),
    AP_INIT_TAKE1("suPHP_PHPPath", suphp_handle_cmd_phppath, NULL, RSRC_CONF,
                  "Path to the PHP binary used to render source view"),
    {NULL}};

/*****************************************
  Code for reading script's stdout/stderr
  based on mod_cgi's code
 *****************************************/

#if APR_FILES_AS_SOCKETS

static const apr_bucket_type_t bucket_type_suphp;

struct suphp_bucket_data {
  apr_pollset_t *pollset;
  request_rec *r;
};

static apr_bucket *suphp_bucket_create(request_rec *r, apr_file_t *out,
                                       apr_file_t *err,
                                       apr_bucket_alloc_t *list) {
  apr_bucket *b = apr_bucket_alloc(sizeof(*b), list);
  apr_pollfd_t fd;
  struct suphp_bucket_data *data = apr_palloc(r->pool, sizeof(*data));

  APR_BUCKET_INIT(b);
  b->free = apr_bucket_free;
  b->list = list;
  b->type = &bucket_type_suphp;
  b->length = (apr_size_t)(-1);
  b->start = (-1);

  /* Create the pollset */
  apr_pollset_create(&data->pollset, 2, r->pool, 0);

  fd.desc_type = APR_POLL_FILE;
  fd.reqevents = APR_POLLIN;
  fd.p = r->pool;
  fd.desc.f = out; /* script's stdout */
  fd.client_data = (void *)1;
  apr_pollset_add(data->pollset, &fd);

  fd.desc.f = err; /* script's stderr */
  fd.client_data = (void *)2;
  apr_pollset_add(data->pollset, &fd);

  data->r = r;
  b->data = data;
  return b;
}

static apr_bucket *suphp_bucket_dup(struct suphp_bucket_data *data,
                                    apr_bucket_alloc_t *list) {
  apr_bucket *b = apr_bucket_alloc(sizeof(*b), list);
  APR_BUCKET_INIT(b);
  b->free = apr_bucket_free;
  b->list = list;
  b->type = &bucket_type_suphp;
  b->length = (apr_size_t)(-1);
  b->start = (-1);
  b->data = data;
  return b;
}

/* This utility method is needed, because APR's implementation for the
   pipe bucket cannot handle or special bucket type                    */
static apr_status_t suphp_read_fd(apr_bucket *b, apr_file_t *fd,
                                  const char **str, apr_size_t *len) {
  char *buf;
  apr_status_t rv;

  *str = NULL;
  *len = APR_BUCKET_BUFF_SIZE;
  buf = apr_bucket_alloc(*len, b->list);

  rv = apr_file_read(fd, buf, len);

  if (*len > 0) {
    /* Got data */
    struct suphp_bucket_data *data = b->data;
    apr_bucket_heap *h;

    /* Change the current bucket to refer to what we read
       and append the pipe bucket after it                */
    b = apr_bucket_heap_make(b, buf, *len, apr_bucket_free);
    /* Here, b->data is the new heap bucket data */
    h = b->data;
    h->alloc_len = APR_BUCKET_BUFF_SIZE; /* note the real buffer size */
    *str = buf;
    APR_BUCKET_INSERT_AFTER(b, suphp_bucket_dup(data, b->list));
  } else {
    /* Got no data */
    apr_bucket_free(buf);
    b = apr_bucket_immortal_make(b, "", 0);
    /* Here, b->data is the reference to the empty string */
    *str = b->data;
  }
  return rv;
}

/* Poll on stdout and stderr to make sure the process does not block
   because of a full system (stderr) buffer                          */
static apr_status_t suphp_bucket_read(apr_bucket *b, const char **str,
                                      apr_size_t *len, apr_read_type_e block) {
  struct suphp_bucket_data *data = b->data;
  apr_interval_time_t timeout;
  apr_status_t rv;
  int gotdata = 0;

  /* Some modules check the length rather than the returned status */
  *len = 0;

  timeout = (block == APR_NONBLOCK_READ) ? 0 : data->r->server->timeout;

  do {
    const apr_pollfd_t *results;
    apr_int32_t num;

    rv = apr_pollset_poll(data->pollset, timeout, &num, &results);
    if (APR_STATUS_IS_TIMEUP(rv)) {
      return (timeout == 0) ? APR_EAGAIN : rv;
    } else if (APR_STATUS_IS_EINTR(rv)) {
      continue;
    } else if (rv != APR_SUCCESS) {
      ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, data->r,
                    "Poll failed waiting for suPHP child process");
      return rv;
    }

    while (num > 0) {
      if (results[0].client_data == (void *)1) {
        /* handle stdout */
        rv = suphp_read_fd(b, results[0].desc.f, str, len);
        if (APR_STATUS_IS_EOF(rv)) {
          rv = APR_SUCCESS;
        }
        gotdata = 1;
      } else {
        /* handle stderr */
        apr_status_t rv2 = suphp_log_script_err(data->r, results[0].desc.f);
        if (APR_STATUS_IS_EOF(rv2)) {
          apr_pollset_remove(data->pollset, &results[0]);
        }
      }
      num--;
      results++;
    }
  } while (!gotdata);

  return rv;
}

static const apr_bucket_type_t bucket_type_suphp = {"SUPHP",
                                                    5,
                                                    APR_BUCKET_DATA,
                                                    apr_bucket_destroy_noop,
                                                    suphp_bucket_read,
                                                    apr_bucket_setaside_notimpl,
                                                    apr_bucket_split_notimpl,
                                                    apr_bucket_copy_notimpl};

#endif

static void suphp_discard_output(apr_bucket_brigade *bb) {
  apr_bucket *b;
  const char *buf;
  apr_size_t len;
  apr_status_t rv;
  for (b = APR_BRIGADE_FIRST(bb); b != APR_BRIGADE_SENTINEL(bb);
       b = APR_BUCKET_NEXT(b)) {
    if (APR_BUCKET_IS_EOS(b)) {
      break;
    }
    rv = apr_bucket_read(b, &buf, &len, APR_BLOCK_READ);
    if (rv != APR_SUCCESS) {
      break;
    }
  }
}

/******************
  Hooks / handlers
 ******************/

static int suphp_script_handler(request_rec *r);
static int suphp_source_handler(request_rec *r);

static int suphp_handler(request_rec *r) {
  suphp_conf *sconf, *dconf;

  sconf = ap_get_module_config(r->server->module_config, &suphp_module);
  dconf = ap_get_module_config(r->per_dir_config, &suphp_module);

  /* only handle request if mod_suphp is active for this handler */
  /* check only first byte of value (second has to be \0) */
  if (apr_table_get(dconf->handlers, r->handler) != NULL) {
    if (*(apr_table_get(dconf->handlers, r->handler)) != '0') {
      return suphp_script_handler(r);
    }
  } else {
    if ((apr_table_get(sconf->handlers, r->handler) != NULL) &&
        (*(apr_table_get(sconf->handlers, r->handler)) != '0')) {
      return suphp_script_handler(r);
    }
  }

  if (!strcmp(r->handler, "x-httpd-php-source") ||
      !strcmp(r->handler, "application/x-httpd-php-source")) {
    return suphp_source_handler(r);
  }

  return DECLINED;
}

static int suphp_source_handler(request_rec *r) {
  suphp_conf *conf;
  apr_status_t rv;
  apr_pool_t *p = r->main ? r->main->pool : r->pool;
  apr_file_t *file;
  apr_proc_t *proc;
  apr_procattr_t *procattr;
  char **argv;
  char **env;
  apr_bucket_brigade *bb;
  apr_bucket *b;
  char *phpexec;
  apr_table_t *empty_table = apr_table_make(p, 0);

  if (strcmp(r->method, "GET")) {
    return DECLINED;
  }

  conf = ap_get_module_config(r->server->module_config, &suphp_module);
  phpexec = apr_pstrdup(p, conf->php_path);
  if (phpexec == NULL) {
    return DECLINED;
  }

  // Try to open file for reading to see whether is is accessible
  rv = apr_file_open(&file, apr_pstrdup(p, r->filename), APR_READ,
                     APR_OS_DEFAULT, p);
  if (rv == APR_SUCCESS) {
    apr_file_close(file);
    file = NULL;
  } else if (rv == EACCES) {
    ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r, "access to %s denied",
                  r->filename);
    return HTTP_FORBIDDEN;
  } else if (rv == ENOENT) {
    ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "File does not exist: %s",
                  r->filename);
    return HTTP_NOT_FOUND;
  } else {
    ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r, "Could not open file: %s",
                  r->filename);
    return HTTP_INTERNAL_SERVER_ERROR;
  }

  /* set attributes for new process */

  if (((rv = apr_procattr_create(&procattr, p)) != APR_SUCCESS) ||
      ((rv = apr_procattr_io_set(procattr, APR_CHILD_BLOCK, APR_CHILD_BLOCK,
                                 APR_CHILD_BLOCK)) != APR_SUCCESS) ||
      ((rv = apr_procattr_dir_set(
            procattr, ap_make_dirstr_parent(r->pool, r->filename))) !=
       APR_SUCCESS) ||
      ((apr_procattr_cmdtype_set(procattr, APR_PROGRAM)) != APR_SUCCESS) ||
      ((apr_procattr_error_check_set(procattr, 1)) != APR_SUCCESS) ||
      ((apr_procattr_detach_set(procattr, 0)) != APR_SUCCESS)) {
    ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                  "couldn't set child process attributes: %s", r->filename);
    return HTTP_INTERNAL_SERVER_ERROR;
  }

  /* create new process */

  argv = apr_palloc(p, 4 * sizeof(char *));
  argv[0] = phpexec;
  argv[1] = "-s";
  argv[2] = apr_pstrdup(p, r->filename);
  argv[3] = NULL;

  env = ap_create_environment(p, empty_table);

  proc = apr_pcalloc(p, sizeof(*proc));
  rv = apr_proc_create(proc, phpexec, (const char *const *)argv,
                       (const char *const *)env, procattr, p);
  if (rv != APR_SUCCESS) {
    ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                  "couldn't create child process: %s for %s", phpexec,
                  r->filename);
    return HTTP_INTERNAL_SERVER_ERROR;
  }
  apr_pool_note_subprocess(p, proc, APR_KILL_AFTER_TIMEOUT);

  if (!proc->out) return APR_EBADF;
  apr_file_pipe_timeout_set(proc->out, r->server->timeout);

  if (!proc->in) return APR_EBADF;
  apr_file_pipe_timeout_set(proc->in, r->server->timeout);

  if (!proc->err) return APR_EBADF;
  apr_file_pipe_timeout_set(proc->err, r->server->timeout);

  /* discard input */

  bb = apr_brigade_create(r->pool, r->connection->bucket_alloc);

  apr_file_flush(proc->in);
  apr_file_close(proc->in);

  rv = ap_get_brigade(r->input_filters, bb, AP_MODE_READBYTES, APR_BLOCK_READ,
                      HUGE_STRING_LEN);
  if (rv != APR_SUCCESS) {
    ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                  "couldn't get input from filters: %s", r->filename);
    return HTTP_INTERNAL_SERVER_ERROR;
  }
  suphp_discard_output(bb);
  apr_brigade_cleanup(bb);

/* get output from script */

#if APR_FILES_AS_SOCKETS
  apr_file_pipe_timeout_set(proc->out, 0);
  apr_file_pipe_timeout_set(proc->err, 0);
  b = suphp_bucket_create(r, proc->out, proc->err, r->connection->bucket_alloc);
#else
  b = apr_bucket_pipe_create(proc->out, r->connection->bucket_alloc);
#endif
  APR_BRIGADE_INSERT_TAIL(bb, b);

  b = apr_bucket_eos_create(r->connection->bucket_alloc);
  APR_BRIGADE_INSERT_TAIL(bb, b);

  /* send output to browser (through filters) */

  r->content_type = "text/html";
  rv = ap_pass_brigade(r->output_filters, bb);

  /* write errors to logfile */

  if (rv == APR_SUCCESS && !r->connection->aborted) {
    suphp_log_script_err(r, proc->err);
    apr_file_close(proc->err);
  }

  return OK;
}

static int suphp_script_handler(request_rec *r) {
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
#if MAX_STRING_LEN < 1024
  char strbuf[1024];
#else
  char strbuf[MAX_STRING_LEN];
#endif
  char *tmpbuf;
  int nph = 0;
  int eos_reached = 0;
  int child_stopped_reading = 0;
  char *auth_user = NULL;
  char *auth_pass = NULL;

  char *ud_user = NULL;
  char *ud_group = NULL;
  int ud_success = 0;
  ap_unix_identity_t *userdir_id = NULL;

  apr_bucket_brigade *bb;
  apr_bucket *b;

  /* load configuration */

  p = r->main ? r->main->pool : r->pool;
  sconf = ap_get_module_config(r->server->module_config, &suphp_module);
  dconf = ap_get_module_config(r->per_dir_config, &suphp_module);
  core_conf =
      (core_dir_config *)ap_get_module_config(r->per_dir_config, &core_module);

  /* check if suPHP is enabled for this request */

  if (((sconf->engine != SUPHP_ENGINE_ON) &&
       (dconf->engine != SUPHP_ENGINE_ON)) ||
      ((sconf->engine == SUPHP_ENGINE_ON) &&
       (dconf->engine == SUPHP_ENGINE_OFF)))
    return DECLINED;

  /* check if file is existing and acessible */

  rv = apr_stat(&finfo, apr_pstrdup(p, r->filename), APR_FINFO_NORM, p);

  if (rv == APR_SUCCESS)
    ; /* do nothing */
  else if (rv == EACCES) {
    return HTTP_FORBIDDEN;
    ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r, "access to %s denied",
                  r->filename);
  } else if (rv == ENOENT) {
    ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "File does not exist: %s",
                  r->filename);
    return HTTP_NOT_FOUND;
  } else {
    ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r, "could not get fileinfo: %s",
                  r->filename);
    return HTTP_NOT_FOUND;
  }

  if (!(r->finfo.protection & APR_UREAD)) {
    ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "Insufficient permissions: %s",
                  r->filename);
    return HTTP_FORBIDDEN;
  }

  /* Check for userdir request */
  userdir_id = ap_run_get_suexec_identity(r);
  if (userdir_id != NULL && userdir_id->userdir) {
    ud_user = apr_psprintf(r->pool, "#%ld", (long)userdir_id->uid);
    ud_group = apr_psprintf(r->pool, "#%ld", (long)userdir_id->gid);
    ud_success = 1;
  }

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
  apr_table_unset(r->subprocess_env, "SUPHP_USER");
  apr_table_unset(r->subprocess_env, "SUPHP_GROUP");
  apr_table_unset(r->subprocess_env, "SUPHP_USERDIR_USER");
  apr_table_unset(r->subprocess_env, "SUPHP_USERDIR_GROUP");

  if (dconf->php_config) {
    apr_table_setn(r->subprocess_env, "SUPHP_PHP_CONFIG",
                   apr_pstrdup(p, dconf->php_config));
  }

  apr_table_setn(r->subprocess_env, "SUPHP_HANDLER", r->handler);

  if (r->headers_in) {
    const char *auth = NULL;
    auth = apr_table_get(r->headers_in, "Authorization");
    if (auth && auth[0] != 0 && strncmp(auth, "Basic ", 6) == 0) {
      char *user;
      char *pass;
      user = ap_pbase64decode(p, auth + 6);
      if (user) {
        pass = strchr(user, ':');
        if (pass) {
          *pass++ = '\0';
          auth_user = apr_pstrdup(r->pool, user);
          auth_pass = apr_pstrdup(r->pool, pass);
        }
      }
    }
  }

  if (auth_user && auth_pass) {
    apr_table_setn(r->subprocess_env, "SUPHP_AUTH_USER", auth_user);
    apr_table_setn(r->subprocess_env, "SUPHP_AUTH_PW", auth_pass);
  }

  if (dconf->target_user) {
    apr_table_setn(r->subprocess_env, "SUPHP_USER",
                   apr_pstrdup(r->pool, dconf->target_user));
  } else if (sconf->target_user) {
    apr_table_setn(r->subprocess_env, "SUPHP_USER",
                   apr_pstrdup(r->pool, sconf->target_user));
  }

  if (dconf->target_group) {
    apr_table_setn(r->subprocess_env, "SUPHP_GROUP",
                   apr_pstrdup(r->pool, dconf->target_group));
  } else if (sconf->target_group) {
    apr_table_setn(r->subprocess_env, "SUPHP_GROUP",
                   apr_pstrdup(r->pool, sconf->target_group));
  }

  if (ud_success) {
    apr_table_setn(r->subprocess_env, "SUPHP_USERDIR_USER",
                   apr_pstrdup(r->pool, ud_user));
    apr_table_setn(r->subprocess_env, "SUPHP_USERDIR_GROUP",
                   apr_pstrdup(r->pool, ud_group));
  }

  env = ap_create_environment(p, r->subprocess_env);

  /* set attributes for new process */

  if (((rv = apr_procattr_create(&procattr, p)) != APR_SUCCESS) ||
      ((rv = apr_procattr_io_set(procattr, APR_CHILD_BLOCK, APR_CHILD_BLOCK,
                                 APR_CHILD_BLOCK)) != APR_SUCCESS) ||
      ((rv = apr_procattr_dir_set(
            procattr, ap_make_dirstr_parent(r->pool, r->filename))) !=
       APR_SUCCESS)
/* set resource limits */

#ifdef RLIMIT_CPU
      || ((rv = apr_procattr_limit_set(procattr, APR_LIMIT_CPU,
                                       core_conf->limit_cpu)) != APR_SUCCESS)
#endif
#if defined(RLIMIT_DATA) || defined(RLIMIT_VMEM) || defined(RLIMIT_AS)
      || ((rv = apr_procattr_limit_set(procattr, APR_LIMIT_MEM,
                                       core_conf->limit_mem)) != APR_SUCCESS)
#endif
#ifdef RLIMIT_NPROC
      || ((apr_procattr_limit_set(procattr, APR_LIMIT_NPROC,
                                  core_conf->limit_nproc)) != APR_SUCCESS)
#endif

      || ((apr_procattr_cmdtype_set(procattr, APR_PROGRAM)) != APR_SUCCESS) ||
      ((apr_procattr_error_check_set(procattr, 1)) != APR_SUCCESS) ||
      ((apr_procattr_detach_set(procattr, 0)) != APR_SUCCESS)) {
    ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                  "couldn't set child process attributes: %s", r->filename);
    return HTTP_INTERNAL_SERVER_ERROR;
  }

  /* create new process */

  proc = apr_pcalloc(p, sizeof(*proc));
  rv = apr_proc_create(proc, SUPHP_PATH_TO_SUPHP, (const char *const *)argv,
                       (const char *const *)env, procattr, p);
  if (rv != APR_SUCCESS) {
    ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                  "couldn't create child process: %s for %s",
                  SUPHP_PATH_TO_SUPHP, r->filename);
    return HTTP_INTERNAL_SERVER_ERROR;
  }
  apr_pool_note_subprocess(p, proc, APR_KILL_AFTER_TIMEOUT);

  if (!proc->out) return APR_EBADF;
  apr_file_pipe_timeout_set(proc->out, r->server->timeout);

  if (!proc->in) return APR_EBADF;
  apr_file_pipe_timeout_set(proc->in, r->server->timeout);

  if (!proc->err) return APR_EBADF;
  apr_file_pipe_timeout_set(proc->err, r->server->timeout);

  /* send request body to script */

  bb = apr_brigade_create(r->pool, r->connection->bucket_alloc);
  do {
    apr_bucket *bucket;
    rv = ap_get_brigade(r->input_filters, bb, AP_MODE_READBYTES, APR_BLOCK_READ,
                        HUGE_STRING_LEN);

    if (rv != APR_SUCCESS) {
      return rv;
    }

    for (bucket = APR_BRIGADE_FIRST(bb); bucket != APR_BRIGADE_SENTINEL(bb);
         bucket = APR_BUCKET_NEXT(bucket)) {
      const char *data;
      apr_size_t len;

      if (APR_BUCKET_IS_EOS(bucket)) {
        eos_reached = 1;
        break;
      }

      if (APR_BUCKET_IS_FLUSH(bucket) || child_stopped_reading) {
        continue;
      }

      apr_bucket_read(bucket, &data, &len, APR_BLOCK_READ);

      rv = apr_file_write_full(proc->in, data, len, NULL);
      if (rv != APR_SUCCESS) {
        child_stopped_reading = 1;
      }
    }
    apr_brigade_cleanup(bb);
  } while (!eos_reached);

  apr_file_flush(proc->in);
  apr_file_close(proc->in);

/* get output from script and check if non-parsed headers are used */

#if APR_FILES_AS_SOCKETS
  apr_file_pipe_timeout_set(proc->out, 0);
  apr_file_pipe_timeout_set(proc->err, 0);
  b = suphp_bucket_create(r, proc->out, proc->err, r->connection->bucket_alloc);
#else
  b = apr_bucket_pipe_create(proc->out, r->connection->bucket_alloc);
#endif

  APR_BRIGADE_INSERT_TAIL(bb, b);

  b = apr_bucket_eos_create(r->connection->bucket_alloc);
  APR_BRIGADE_INSERT_TAIL(bb, b);

  tmpbuf = suphp_brigade_read(p, bb, 8);
  if (strlen(tmpbuf) == 8 &&
      !(strncmp(tmpbuf, "HTTP/1.0", 8) && strncmp(tmpbuf, "HTTP/1.1", 8))) {
    nph = 1;
  }

  if (!nph) {
    /* normal cgi headers, so we have to create the real headers ourselves */

    int ret;
    const char *location;

    ret = ap_scan_script_header_err_brigade(r, bb, strbuf);
    if (ret == HTTP_NOT_MODIFIED) {
      return ret;
    } else if (ret != APR_SUCCESS) {
      suphp_discard_output(bb);
      apr_brigade_destroy(bb);
      suphp_log_script_err(r, proc->err);

      /* ap_scan_script_header_err_brigade does logging itself,
         so simply return                                       */

      return HTTP_INTERNAL_SERVER_ERROR;
    }

    location = apr_table_get(r->headers_out, "Location");
    if (location && location[0] == '/' && r->status == 200) {
      /* empty brigade (script output) and modify headers */

      suphp_discard_output(bb);
      apr_brigade_destroy(bb);
      suphp_log_script_err(r, proc->err);
      r->method = apr_pstrdup(r->pool, "GET");
      r->method_number = M_GET;
      apr_table_unset(r->headers_in, "Content-Length");

      ap_internal_redirect_handler(location, r);
      return OK;
    } else if (location && r->status == 200) {
      /* empty brigade (script output) */
      suphp_discard_output(bb);
      apr_brigade_destroy(bb);
      suphp_log_script_err(r, proc->err);
      return HTTP_MOVED_TEMPORARILY;
    }

    /* send output to browser (through filters) */

    rv = ap_pass_brigade(r->output_filters, bb);

    /* write errors to logfile */

    if (rv == APR_SUCCESS && !r->connection->aborted)
      suphp_log_script_err(r, proc->err);

    apr_file_close(proc->err);
  }

  if (proc->out && nph) {
    /* use non-parsed headers (direct output) */

    struct ap_filter_t *cur;

    /* get rid of output filters */

    cur = r->proto_output_filters;
    while (cur && cur->frec->ftype < AP_FTYPE_CONNECTION) {
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

static void suphp_register_hooks(apr_pool_t *p) {
  ap_hook_handler(suphp_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

/********************
  Module declaration
 ********************/
/* clang-format off */
module AP_MODULE_DECLARE_DATA suphp_module = {
  STANDARD20_MODULE_STUFF,
  suphp_create_dir_config,
  suphp_merge_dir_config,
  suphp_create_server_config,
  suphp_merge_server_config,
  suphp_cmds,
  suphp_register_hooks
};
