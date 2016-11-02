/*=============================================================================
#     FileName: ngx_http_mmseg_module.h
#         Desc: handler模块 配置相关
#       Author: Daniel
#        Email: daneustc@gmail.com
#     HomePage: http://ustcdane.github.io/
#      Version: 0.0.1
#   LastChange: 2016-11-01 19:25:21
#      History:
=============================================================================*/
#ifndef HTTP_MMSEG_MODULE_H_
#define HTTP_MMSEG_MODULE_H_

//从conf文件中读取的参数，路径参数等
#ifdef __cplusplus
extern "C" {
#endif
	
/*
typedef struct
{
	ngx_str_t words_path; // 字典文件路径
	ngx_str_t charFreq_path;// 单字词频路径
} ngx_http_mmseg_loc_conf_t;
*/

typedef struct {
	    ngx_str_t output_words;
} ngx_http_mmseg_loc_conf_t;

//ngx_http_mmseg_loc_conf_t* get_mmseg_loc_conf(ngx_http_request_t *r);//通过request 获得配置信息
#ifdef __cplusplus
}
#endif
//ngx_str_t ngx_parser_http_request(ngx_http_request_t *r);// 获得http请求参数
#endif
