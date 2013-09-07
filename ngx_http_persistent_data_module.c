
/*
 * Copyright (C) Jonathan Kolb
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>


typedef struct {
    ngx_int_t   index;
} ngx_http_persistent_data_conf_t;


typedef struct {
    ngx_int_t   random;
} ngx_http_persistent_data_t;


static ngx_int_t ngx_http_variable_not_found(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_persistent_data_add_variable(ngx_conf_t *cf);
static void *ngx_http_persistent_data_create_main_conf(ngx_conf_t *cf);
static ngx_int_t ngx_http_persistent_data_write_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_persistent_data_read_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_persistent_data_init(ngx_conf_t *cf);


static ngx_http_module_t  ngx_http_persistent_data_module_ctx = {
    ngx_http_persistent_data_add_variable,     /* preconfiguration */
    ngx_http_persistent_data_init,             /* postconfiguration */

    ngx_http_persistent_data_create_main_conf, /* create main configuration */
    NULL,                                      /* init main configuration */

    NULL,                                      /* create server configuration */
    NULL,                                      /* merge server configuration */

    NULL,                                      /* create location configuration */
    NULL                                       /* merge location configuration */
};


ngx_module_t  ngx_http_persistent_data_module = {
    NGX_MODULE_V1,
    &ngx_http_persistent_data_module_ctx,  /* module context */
    NULL,                                  /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_str_t persistent_data_varname = ngx_string("persistent_data_var");


static ngx_int_t
ngx_http_variable_not_found(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data)
{
    v->not_found = 1;
    return NGX_OK;
}


static ngx_int_t
ngx_http_persistent_data_add_variable(ngx_conf_t *cf)
{
    ngx_http_variable_t                *var;

    var = ngx_http_add_variable(cf, &persistent_data_varname,
        NGX_HTTP_VAR_INDEXED|NGX_HTTP_VAR_NOHASH);
    if (NULL == var) {
        return NGX_ERROR;
    }

    var->get_handler = ngx_http_variable_not_found;

    return NGX_OK;
}


static void *
ngx_http_persistent_data_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_persistent_data_conf_t    *pdcf;

    pdcf = ngx_pcalloc(cf->pool, sizeof(ngx_http_persistent_data_conf_t));
    if (NULL == pdcf) {
        return NULL;
    }

    /*
     * set by ngx_pcalloc():
     *
     *     pdcf->index = 0;
     */

    return pdcf;
}


static ngx_int_t
ngx_http_persistent_data_write_handler(ngx_http_request_t *r)
{
    ngx_http_persistent_data_conf_t    *pdcf;
    ngx_http_variable_value_t          *v;

    pdcf = ngx_http_get_module_main_conf(r, ngx_http_persistent_data_module);

    v = &r->variables[pdcf->index];
    if (NULL == v->data) {
        ngx_http_persistent_data_t *pd;

        pd = ngx_palloc(r->pool, sizeof(ngx_http_persistent_data_t));
        if (NULL == pd) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        pd->random = ngx_random();
        ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
            "setting pd->random to %i", pd->random);
        v->data = (void *) pd;

    } else {
        ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "internal redirect!");
    }

    return NGX_DECLINED;
}


static ngx_int_t
ngx_http_persistent_data_read_handler(ngx_http_request_t *r)
{
    ngx_http_persistent_data_conf_t    *pdcf;
    ngx_http_variable_value_t          *v;

    pdcf = ngx_http_get_module_main_conf(r, ngx_http_persistent_data_module);

    v = &r->variables[pdcf->index];
    if (NULL == v->data) {
        ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
            "persistent data value not set");

    } else {
        ngx_http_persistent_data_t *pd;

        pd = (void *) v->data;
        ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
            "pd->random is %i", pd->random);
    }

    return NGX_DECLINED;
}


static ngx_int_t
ngx_http_persistent_data_init(ngx_conf_t *cf)
{
    ngx_http_persistent_data_conf_t    *pdcf;
    ngx_http_core_main_conf_t          *cmcf;
    ngx_http_handler_pt                *h;

    pdcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_persistent_data_module);
    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    pdcf->index = ngx_http_get_variable_index(cf, &persistent_data_varname);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_persistent_data_write_handler;

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_LOG_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_persistent_data_read_handler;

    return NGX_OK;
}

