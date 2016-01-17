/*=============================================================================
#     FileName: ngx_http_handle_interface.h
#         Desc: 处理HTTP请求的接口
#       Author: Daniel
#        Email: daneustc@gmail.com
#     HomePage: http://ustcdane.github.io/
#      Version: 0.0.1
#   LastChange: 2016-01-17 10:26:50
#      History:
=============================================================================*/
#ifndef NGINX_HTTP_HANDLE_INTERFACE_H_
#define NGINX_HTTP_HANDLE_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

int ngx_http_do_get(ngx_http_request_t *r);
void ngx_http_do_post(ngx_http_request_t *r);
int ngx_http_invalid_request(ngx_http_request_t *r);

#ifdef __cplusplus
}
#endif

#endif
