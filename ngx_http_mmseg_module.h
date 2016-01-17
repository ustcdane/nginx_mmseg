/*=============================================================================
#     FileName: ngx_http_mmseg_module.h
#         Desc: handlerģ�� �������
#       Author: Daniel
#        Email: daneustc@gmail.com
#     HomePage: http://ustcdane.github.io/
#      Version: 0.0.1
#   LastChange: 2016-01-17 10:32:30
#      History:
=============================================================================*/
#ifndef HTTP_MMSEG_MODULE_H_
#define HTTP_MMSEG_MODULE_H_

//��conf�ļ��ж�ȡ�Ĳ�����·��������
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	ngx_str_t words_path; // �ֵ��ļ�·��
	ngx_str_t charFreq_path;// ���ִ�Ƶ·��
} ngx_http_mmseg_loc_conf_t;

ngx_http_mmseg_loc_conf_t* get_mmseg_loc_conf(ngx_http_request_t *r);//ͨ��request ���������Ϣ
ngx_str_t ngx_parser_http_request(ngx_http_request_t *r);// ���http�������
#ifdef __cplusplus
}
#endif

#endif
