#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "ngx_http_mmseg_module.h"
#include "ngx_http_handle_interface.h"


//初始化这个模块的函数，在后面有定义
static ngx_int_t ngx_http_mmseg_init(ngx_conf_t *cf);

//这个函数创建了本区域的配置信息
//nginx的配置信息分为三层
//第一层是main配置，比如说work process数量这类，
//第二层是主模块配置，比如说http模块的配置，在http{ 里面
//第三层是区域的配置信息，在http的server里对应的每个具体路径下的配置信息，也就是这个函数读取的配置
static void *ngx_http_mmseg_create_loc_conf(ngx_conf_t *cf);

//获取conf中参数的函数，一般名称和参数名一致
static char *ngx_http_words_path(ngx_conf_t *cf, ngx_command_t *cmd,
        void *conf);
static char *ngx_http_charFreq_path(ngx_conf_t *cf, ngx_command_t *cmd,
        void *conf);


//这个结构体是用来配置参数的信息的
//第一项是参数的名称，必须和conf文件中写的一样
//第二项表示参数的类型，如NGX_HTTP_LOC_CONF表示这个是区域配置信息，
//NGX_CONF_TAKE1表示必须参数跟着1个值，同理如果是TAKE2表示必须跟着两个值，这个可以上网去查
//第三项表示获取该参数的函数指针
//第四项表示OFFSET类型，区域配置用这个即可
//第五项表示offset的位置，照着写就好，
//第六项一般是NULL，是个回调函数指针
//定义了两个配置指令 words_path,charFreq_path,其实一个配置指令就能完成任务,为了演示多个配置指令的使用方法
static ngx_command_t ngx_http_mmseg_commands[] = {
	{
		ngx_string("words_path"),
		NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,//  NGX_CONF_TAKE3..
		ngx_http_words_path,// 当nginx在解析配置的时候，如果遇到这个配置指令，将会把读取到的值传递给这个函数进行分解处理。
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_mmseg_loc_conf_t, words_path),// 一般指定为某一个结构体变量的字段偏移
		NULL 
	},
	
	{
		ngx_string("charFreq_path"),
		NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
		ngx_http_charFreq_path,
		NGX_HTTP_LOC_CONF_OFFSET,
		offsetof(ngx_http_mmseg_loc_conf_t, charFreq_path),
		NULL
	},
	ngx_null_command // 注意写法
};
/*
也可以master就加载词库等信息，子进程worker共享父进程数据，copy on write
词库较大时 这种方法更好！

typedef struct {
    ngx_str_t output_words;
} ngx_http_mmseg_loc_conf_t;

static ngx_command_t ngx_http_mmseg_commands[] = { 
    {   
        ngx_string("nginx_mmseg"), // The command name
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE3,
        ngx_http_mmseg_set_conf, // The command handler
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_mmseg_loc_conf_t, output_words),
        NULL
    },  
    ngx_null_command
};
// 这里加载词库数据
MMSeg g_interface;
static char* ngx_http_mmseg_set_conf(ngx_conf_t* cf, ngx_command_t* cmd, void* conf) {
   ngx_http_core_loc_conf_t* clcf;
    clcf = (ngx_http_core_loc_conf_t*)ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_mmseg_handler;
    ngx_conf_set_str_slot(cf, cmd, conf);
    if (cf->args->nelts != 3) {
        ngx_log_error(NGX_LOG_ERR, cf->log, 0, " [the number of conf'a args is not 3] ");
        return (char*)NGX_CONF_ERROR;
    }   
    ngx_str_t * value = (ngx_str_t *)cf->args->elts;

    g_interface = MMSeg::Instance(words_p, chars_p);
    return NGX_CONF_OK;
}
/*



// 模块上下文结构
// 设置module启动时需要执行的一些函数
static ngx_http_module_t ngx_http_mmseg_module_ctx = {
        NULL,                          /* preconfiguration */
        ngx_http_mmseg_init,           /* postconfiguration */
        NULL,                          /* create main configuration */
        NULL,                          /* init main configuration */
        NULL,                          /* create server configuration */
        NULL,                          /* merge server configuration */
        ngx_http_mmseg_create_loc_conf, /* create location configuration */
        NULL                            /* merge location configuration */
};

// 模块的定义
// 它告诉nginx这个模块的一些信息,上下文信息,配置指令等等
ngx_module_t ngx_http_mmseg_module = {
        NGX_MODULE_V1,
        &ngx_http_mmseg_module_ctx,    /* module context */
        ngx_http_mmseg_commands,       /* module directives */
        NGX_HTTP_MODULE,               /* module type */
        NULL,                          /* init master */
        NULL,                          /* init module */
        NULL,                          /* init process */
        NULL,                          /* init thread */
        NULL,                          /* exit thread */
        NULL,                          /* exit process */
        NULL,                          /* exit master */
        NGX_MODULE_V1_PADDING
};

//解析url中的参数,主要处理 http get 请求
inline ngx_str_t ngx_parser_http_request(ngx_http_request_t *r) {
	 ngx_str_t value;
	  // args is data=xxxxx
	 // 第二个参数是HTTP GET的key名,key名的长度,第四个参数是想存这个value的变量的指针
	 if (NGX_OK != ngx_http_arg(r, (u_char*)"data", 4, &value)) {
		 ngx_str_t res = ngx_null_string;
		 return res;
	 }
	 
	 ngx_str_t sentence;
	 u_char* dst, *src;
	 dst = (u_char*)ngx_pcalloc(r->pool, 1024);//解码后字词空间
	 src = value.data;
	 sentence.data = dst;
	 // 解析url 编码
	 ngx_unescape_uri(&dst, &src, value.len, 0);
	 sentence.len = dst - sentence.data;
	 //ngx_log_error(NGX_LOG_EMERG, r->connection->log, 0, " data=%s", sentence.data);
	 return sentence; 
}
// 通过r获得ngx_http_mmseg_loc_conf_t 配置信息
inline ngx_http_mmseg_loc_conf_t* get_mmseg_loc_conf(ngx_http_request_t *r) {
	return (ngx_http_mmseg_loc_conf_t*)ngx_http_get_module_loc_conf(r, ngx_http_mmseg_module);
}


//这里是处理每个请求的回调函数
//执行到这一步时，nginx已经把http请求做了初步的解析，整个请求的信息放在ngx_http_request_t中
static ngx_int_t
ngx_http_mmseg_handler(ngx_http_request_t *r) {
	ngx_int_t    rc;
	
    //根据请求类型的不同执行不同动作
	if (r->method & NGX_HTTP_GET) { // get
		return ngx_http_do_get(r);// get
		
    } else if (r->method & NGX_HTTP_POST) { // post
		rc = ngx_http_read_client_request_body(r, ngx_http_do_post); 
			//ngx_http_do_post(&request_info);
		if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
			return rc;
		}
		return NGX_DONE;

	} else {
		//nginx的自带函数，表示抛弃包体，不执行任何操作
		rc = ngx_http_discard_request_body(r);
		return rc;
	}

	return NGX_HTTP_NOT_ALLOWED;
}

static void *ngx_http_mmseg_create_loc_conf(ngx_conf_t *cf)
{
        ngx_http_mmseg_loc_conf_t* local_conf = NULL;
        local_conf = (ngx_http_mmseg_loc_conf_t*)ngx_pcalloc(cf->pool, sizeof(ngx_http_mmseg_loc_conf_t));
        if (local_conf == NULL)
        {
                return NULL;
        }

        ngx_str_null(&local_conf->words_path);
        ngx_str_null(&local_conf->charFreq_path);

        return local_conf;
}

//  配置指令处理函数,定义原型如下:
// char *(*set)(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
// set这是一个函数指针，当nginx在解析配置的时候，如果遇到这个配置指令，
// 将会把读取到的值传递给这个函数进行分解处理。
// cf: 该参数里面保存从配置文件读取到的原始字符串以及相关的一些信息。
// cmd: 这个配置指令对应的ngx_command_t结构。
// conf: 就是定义的存储这个配置值的结构体，比如ngx_http_mmseg_loc_conf_t。
// 当解析这个words_path变量的时候，传入的conf就指向一个ngx_http_mmseg_loc_conf_t
// 类型的变量。用户在处理的时候可以使用类型转换，转换成自己知道的类型，再进行字段的赋值。
static char *
ngx_http_words_path(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
        ngx_http_mmseg_loc_conf_t* local_conf;
        local_conf = (ngx_http_mmseg_loc_conf_t*)conf;
        char* rv = ngx_conf_set_str_slot(cf, cmd, conf);
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "words path is :%s", local_conf->words_path.data);
        return rv;
}


static char *
ngx_http_charFreq_path(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
        ngx_http_mmseg_loc_conf_t* local_conf;
        local_conf = (ngx_http_mmseg_loc_conf_t*)conf;
        char* rv = ngx_conf_set_str_slot(cf, cmd, conf);
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "char freq path:%s", local_conf->charFreq_path.data);
        return rv;
}

// handler模块的挂载 大多数是挂载在NGX_HTTP_CONTENT_PHASE阶段的
// 挂载的动作一般是在模块上下文调用的postconfiguration函数中
static ngx_int_t
ngx_http_mmseg_init(ngx_conf_t *cf)
{
        ngx_http_handler_pt        *h;
        ngx_http_core_main_conf_t  *cmcf;

        cmcf = (ngx_http_core_main_conf_t*)ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

        h = (ngx_http_handler_pt*)ngx_array_push(&cmcf->phases[NGX_HTTP_CONTENT_PHASE].handlers);
        if (h == NULL) {
                return NGX_ERROR;
        }

        *h = ngx_http_mmseg_handler;// 注册对请求处理函数

        return NGX_OK;
}
