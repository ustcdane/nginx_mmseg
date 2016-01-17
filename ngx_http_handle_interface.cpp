#include <string>
#include "ngx_http_handle_interface.h"
#include "ngx_http_mmseg_module.h"
#include "Mmseg.h"
using namespace mmsegSpace;

// ngx_str_t to std::string
inline std::string ngx_str_to_string(ngx_str_t str) {
	std::string src((const char*)str.data, str.len);
	return src;
}

int ngx_http_do_get(ngx_http_request_t *r) {
	
	ngx_int_t    rc; 
	ngx_chain_t  out;
	//  
	ngx_http_mmseg_loc_conf_t* myconf = get_mmseg_loc_conf(r);
	// 待分词的句子
	ngx_str_t sentence = ngx_parser_http_request(r);// ngx_str_t
	
	if (sentence.len == 0) {
		return ngx_http_invalid_request(r);	
	}
	
	ngx_str_t words_path = myconf->words_path;// 字典路径
	ngx_str_t charFreq_path = myconf->charFreq_path;// 单字词频路径
	
	ngx_log_error(NGX_LOG_EMERG, r->connection->log, 0, "Dict Path is %s\n", (char*)words_path.data);
	std::string words_p = ngx_str_to_string(words_path),chars_p = ngx_str_to_string(charFreq_path);
	MMSeg& interface = MMSeg::Instance(words_p, chars_p);
	std::string str = ngx_str_to_string(sentence);
	ngx_log_error(NGX_LOG_EMERG, r->connection->log, 0, "sentence is: %s\n", sentence.data);
	std::u16string s = TransCode::from_utf8(trim(str));
	std::string result;
	for (auto& w: interface.segment(s)) 
		result += TransCode::to_utf8(w) + " ";
	trim(result);
	ngx_log_error(NGX_LOG_EMERG, r->connection->log, 0, "result is: %s\n", result.c_str());
	
	ngx_str_t type = ngx_string("text/plain");
	ngx_str_t resp;
	if (result.empty()) {
		resp.data = (u_char*)"";
		resp.len = 0;
	} else {
		resp.data = (u_char*)result.c_str();
		resp.len = result.size();
	}
	
	//将返回的Header信息写入r中
	r->headers_out.status = NGX_HTTP_OK;
	r->headers_out.content_length_n = resp.len;
	r->headers_out.content_type = type;

	//用来存储返回内容的buf，用nginx的内存池来创建
	ngx_buf_t* b = ngx_create_temp_buf(r->pool, resp.len);
	if (b == NULL) {
		return ngx_http_invalid_request(r);
	}
	//buf可以有很多块，如果有多块，需要在out中指定next指针指向下一块
	//如果到了最后一块，那么把last_buf设置为1
	ngx_memcpy(b->pos, resp.data, resp.len);
	//这里的last表示这块buf的终止指针位置
	b->last = b->pos + resp.len;
	b->last_buf = 1;
	
	out.buf = b;
	out.next = NULL;
	
	rc = ngx_http_send_header(r);
	if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
		return rc;
	}
	// send the buffer chain of your response 
	return ngx_http_output_filter(r, &out);
}

// 获得post 内容
static ngx_int_t get_post_content(ngx_http_request_t *r, char * data_buf, size_t content_length) {
	ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "[get_post_content] [content_length:%d]", content_length); //DEBUG
	
	ngx_chain_t* bufs = r->request_body->bufs;
	ngx_buf_t* buf = NULL;
	size_t body_length = 0;
	size_t buf_length;
	while(bufs) {
		buf = bufs->buf;
		bufs = bufs->next;
		buf_length = buf->last - buf->pos;
		if(body_length + buf_length > content_length) {
			memcpy(data_buf + body_length, buf->pos, content_length - body_length);
			body_length = content_length;
			break;
		}
		memcpy(data_buf + body_length, buf->pos, buf->last - buf->pos);
		body_length += buf->last - buf->pos;
	}
	if(body_length != content_length) {
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "get_post_content's body_length != content_length in headers");
		return NGX_ERROR;
	}
	return NGX_OK;
}

// 响应post请求
static ngx_int_t ngx_http_mmseg_send_response(ngx_http_request_t * r, const char* type, const char* data_buf, size_t len) {
	ngx_int_t rc;
	ngx_buf_t* b;
	ngx_chain_t out;
	
	b = ngx_create_temp_buf(r->pool, len);
	if (b == NULL) {
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	
	ngx_memcpy(b->pos, data_buf, len);
	b->last = b->pos + len;
	b->last_buf = 1;
	
	out.buf = b;
	out.next = NULL;

	r->headers_out.status = NGX_HTTP_OK;
	r->headers_out.content_length_n = len;
	r->headers_out.content_type.data = (u_char*) type;
	r->headers_out.content_type.len = strlen(type);
	
	rc = ngx_http_send_header(r);
	if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
		return rc;
	}
	return ngx_http_output_filter(r, &out);
}

// post handle
void ngx_http_do_post(ngx_http_request_t *r) {

	ngx_http_mmseg_loc_conf_t* myconf = get_mmseg_loc_conf(r);

	 if(r->headers_in.content_length_n == 0) {
		 ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "r->headers_in.content_length_n is 0");
		 ngx_http_finalize_request(r, NGX_ERROR);
		 return ;
	 }

	//ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "r->headers_in.content_length_n is:%d\n", r->headers_in.content_length_n);
	
	ngx_int_t rc;
	char * data_buf = NULL;
	data_buf = (char*) ngx_pcalloc(r->pool, r->headers_in.content_length_n + 1);// content_length_n + 1
	if (data_buf == NULL) {
		ngx_http_finalize_request(r, NGX_ERROR);
		return ;
	}
	
	if (NGX_ERROR == get_post_content(r, data_buf, r->headers_in.content_length_n)) {
		ngx_http_finalize_request(r, NGX_ERROR);
		return ;
	}

	ngx_str_t words_path = myconf->words_path;// 字典路径
	ngx_str_t charFreq_path = myconf->charFreq_path;// 单字词频路径
	
	//ngx_log_error(NGX_LOG_EMERG, r->connection->log, 0, "Dict Path is %s\n", (char*)words_path.data);
	std::string words_p = ngx_str_to_string(words_path),chars_p = ngx_str_to_string(charFreq_path);
	MMSeg& interface = MMSeg::Instance(words_p, chars_p);
	
	ngx_str_t sentence;
	u_char* dst, *src;
	dst = (u_char*)ngx_pcalloc(r->pool, r->headers_in.content_length_n + 1);//解码后字词空间
	src = (u_char*)data_buf;
	sentence.data = dst;
	// 解析url 编码
	ngx_unescape_uri(&dst, &src, r->headers_in.content_length_n, 0); 
	sentence.len = dst - sentence.data;
	
	std::string str((const char*)sentence.data, sentence.len);// 转码后数据
	ngx_log_error(NGX_LOG_EMERG, r->connection->log, 0, "sentence is: %s\n", sentence.data);
	std::u16string s = TransCode::from_utf8(trim(str));
	std::string result;
	for (auto& w: interface.segment(s)) 
		result += TransCode::to_utf8(w) + " ";
	trim(result);
	ngx_log_error(NGX_LOG_EMERG, r->connection->log, 0, "result is: %s\n", result.c_str());
	
	rc = ngx_http_mmseg_send_response(r, "text/plain", result.c_str(), result.size());
	ngx_log_error(NGX_LOG_INFO, r->connection->log, 0, "[ngx_http_mmseg_send_response] [response size:%d]", result.size());
	 ngx_http_finalize_request(r, rc);
}

int ngx_http_invalid_request(ngx_http_request_t *r) {
	ngx_str_t type = ngx_string("text/plain");

	r->headers_out.status = NGX_HTTP_BAD_REQUEST;
	r->headers_out.content_length_n = 0;
	r->headers_out.content_type = type;
	r->header_only = 1;
	return -1;
}
