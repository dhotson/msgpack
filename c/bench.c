#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <msgpack/pack.h>
#include <msgpack/unpack.h>
#include <yajl/yajl_parse.h>
#include <yajl/yajl_gen.h>


static struct timeval g_timer;

void reset_timer()
{
	gettimeofday(&g_timer, NULL);
}

void show_timer(size_t bufsz)
{
	struct timeval endtime;
	gettimeofday(&endtime, NULL);
	double sec = (endtime.tv_sec - g_timer.tv_sec)
		+ (double)(endtime.tv_usec - g_timer.tv_usec) / 1000 / 1000;
	printf("%f sec\n", sec);
	printf("%f MB\n", ((double)bufsz)/1024/1024);
	printf("%f Mbps\n", ((double)bufsz)*8/sec/1000/1000);
}


static int reformat_null(void * ctx) { return 1; }
static int reformat_boolean(void * ctx, int boolean) { return 1; }
static int reformat_number(void * ctx, const char * s, unsigned int l) { return 1; }
static int reformat_string(void * ctx, const unsigned char * stringVal, unsigned int stringLen) { return 1; }
static int reformat_map_key(void * ctx, const unsigned char * stringVal, unsigned int stringLen) { return 1; }
static int reformat_start_map(void * ctx) { return 1; }
static int reformat_end_map(void * ctx) { return 1; }
static int reformat_start_array(void * ctx) { return 1; }
static int reformat_end_array(void * ctx) { return 1; }


static void* unpack_uint8(void* data, uint8_t d) { return NULL; }
static void* unpack_uint16(void* data, uint16_t d) { return NULL; }
static void* unpack_uint32(void* data, uint32_t d) { return NULL; }
static void* unpack_uint64(void* data, uint64_t d) { return NULL; }
static void* unpack_int8(void* data, int8_t d) { return NULL; }
static void* unpack_int16(void* data, int16_t d) { return NULL; }
static void* unpack_int32(void* data, int32_t d) { return NULL; }
static void* unpack_int64(void* data, int64_t d) { return NULL; }
static void* unpack_float(void* data, float d) { return NULL; }
static void* unpack_double(void* data, double d) { return NULL; }
static void* unpack_nil(void* data) { return NULL; }
static void* unpack_true(void* data) { return NULL; }
static void* unpack_false(void* data) { return NULL; }
static void* unpack_array(void* data, unsigned int n) { return NULL; }
static void unpack_array_item(void* data, void* c, void* o) { }
static void* unpack_map(void* data, unsigned int n) { return NULL; }
static void unpack_map_item(void* data, void* c, void* k, void* v) { }
static void* unpack_raw(void* data, const char* b, const char* p, unsigned int l) { /*printf("unpack raw %p %lu\n",p,l);*/ return NULL; }


typedef struct {
	size_t allocated;
	size_t length;
	char* buffer;
} pack_buffer;

static const size_t PACK_INITIAL_BUFFER_SIZE = 32*1024;

static void pack_buffer_init(pack_buffer* data)
{
	data->buffer = malloc(PACK_INITIAL_BUFFER_SIZE);
	data->length = 0;
	data->allocated = PACK_INITIAL_BUFFER_SIZE;
}

static void pack_buffer_reset(pack_buffer* data)
{
	data->buffer = realloc(data->buffer, PACK_INITIAL_BUFFER_SIZE);
	data->allocated = PACK_INITIAL_BUFFER_SIZE;
	data->length = 0;
}

static void pack_buffer_free(pack_buffer* data)
{
	free(data->buffer);
}

static void pack_append_buffer(void* user, const char* b, unsigned int l)
{
	pack_buffer* data = (pack_buffer*)user;
	if(data->allocated - data->length < l) {
		data->buffer = realloc(data->buffer, data->allocated*2);
		data->allocated *= 2;
	}
	memcpy(data->buffer + data->length, b, l);
	data->length += l;
}


static const unsigned int TASK_INT_NUM = 1<<24;
static const unsigned int TASK_STR_LEN = 1<<15;
//static const unsigned int TASK_INT_NUM = 1<<20;
//static const unsigned int TASK_STR_LEN = 1<<12;
static const char* TASK_STR_PTR;


void bench_json(void)
{
	puts("== JSON ==");


	yajl_gen_config gcfg = {0, NULL};
	yajl_gen g = yajl_gen_alloc(&gcfg);

	yajl_parser_config hcfg = { 0, 0 };
	yajl_callbacks callbacks = {
	    reformat_null,
	    reformat_boolean,
	    NULL,
	    NULL,
	    reformat_number,
		reformat_string,
	    reformat_start_map,
	    reformat_map_key,
	    reformat_end_map,
	    reformat_start_array,
	    reformat_end_array
	};
	yajl_handle h = yajl_alloc(&callbacks, &hcfg, NULL);


	const unsigned char * buf;
	unsigned int len;


	puts("generate integer");
	reset_timer();
	{
		unsigned int i;
		yajl_gen_array_open(g);
		for(i=0; i < TASK_INT_NUM; ++i) {
			yajl_gen_integer(g, i);
		}
		yajl_gen_array_close(g);
	}
	show_timer(len);

	yajl_gen_get_buf(g, &buf, &len);

	puts("----");
	puts("parse integer");
	reset_timer();
	{
		yajl_status stat = yajl_parse(h, buf, len);
		if (stat != yajl_status_ok && stat != yajl_status_insufficient_data) {
			unsigned char * str = yajl_get_error(h, 1, buf, len);
			fprintf(stderr, (const char *) str);
		}
	}
	show_timer(len);


	//yajl_gen_clear(g);
	yajl_gen_free(g);
	g = yajl_gen_alloc(&gcfg);
	yajl_free(h);
	h = yajl_alloc(&callbacks, &hcfg, NULL);


	puts("----");
	puts("generate string");
	reset_timer();
	{
		unsigned int i;
		yajl_gen_array_open(g);
		for(i=0; i < TASK_STR_LEN; ++i) {
			yajl_gen_string(g, (const unsigned char*)TASK_STR_PTR, i);
		}
		yajl_gen_array_close(g);
	}
	show_timer(len);

	yajl_gen_get_buf(g, &buf, &len);

	puts("----");
	puts("parse string");
	reset_timer();
	{
		yajl_status stat = yajl_parse(h, buf, len);
		if (stat != yajl_status_ok && stat != yajl_status_insufficient_data) {
			unsigned char * str = yajl_get_error(h, 1, buf, len);
			fprintf(stderr, (const char *) str);
		}
	}
	show_timer(len);


	yajl_gen_free(g);
	yajl_free(h);
}


void bench_msgpack(void)
{
	puts("== MessagePack ==");


	pack_buffer mpkbuf;
	pack_buffer_init(&mpkbuf);

	msgpack_pack_t* mpk = msgpack_pack_new(
			&mpkbuf, pack_append_buffer);

	msgpack_unpack_callback cb = {
		unpack_uint8,
		unpack_uint16,
		unpack_uint32,
		unpack_uint64,
		unpack_int8,
		unpack_int16,
		unpack_int32,
		unpack_int64,
		unpack_float,
		unpack_double,
		unpack_nil,
		unpack_true,
		unpack_false,
		unpack_array,
		unpack_array_item,
		unpack_map,
		unpack_map_item,
		unpack_raw,
	};
	msgpack_unpack_t* mupk = msgpack_unpack_new(NULL, &cb);


	size_t len;
	const char* buf;


	puts("pack integer");
	reset_timer();
	{
		unsigned int i;
		msgpack_pack_array(mpk, TASK_INT_NUM);
		for(i=0; i < TASK_INT_NUM; ++i) {
			msgpack_pack_unsigned_int(mpk, i);
		}
	}
	show_timer(mpkbuf.length);

	len = mpkbuf.length;
	buf = mpkbuf.buffer;

	puts("----");
	puts("unpack integer");
	reset_timer();
	{
		size_t off = 0;
		int ret = msgpack_unpack_execute(mupk, buf, len, &off);
		if(ret < 0) {
			fprintf(stderr, "Parse error.\n");
		} else if(ret == 0) {
			fprintf(stderr, "Not finished.\n");
		}
	}
	show_timer(mpkbuf.length);


	pack_buffer_reset(&mpkbuf);
	msgpack_unpack_reset(mupk);


	puts("----");
	puts("pack string");
	reset_timer();
	{
		unsigned int i;
		msgpack_pack_array(mpk, TASK_STR_LEN);
		for(i=0; i < TASK_STR_LEN; ++i) {
			msgpack_pack_raw(mpk, i);
			msgpack_pack_raw_body(mpk, TASK_STR_PTR, i);
		}
	}
	show_timer(mpkbuf.length);

	len = mpkbuf.length;
	buf = mpkbuf.buffer;

	puts("----");
	puts("unpack string");
	reset_timer();
	{
		size_t off = 0;
		int ret = msgpack_unpack_execute(mupk, buf, len, &off);
		if(ret < 0) {
			fprintf(stderr, "Parse error.\n");
		} else if(ret == 0) {
			fprintf(stderr, "Not finished.\n");
		}
	}
	show_timer(mpkbuf.length);


	msgpack_unpack_free(mupk);
	msgpack_pack_free(mpk);
	pack_buffer_free(&mpkbuf);
}

int main(int argc, char* argv[])
{
	char* str = malloc(TASK_STR_LEN);
	memset(str, 'a', TASK_STR_LEN);
	TASK_STR_PTR = str;

	bench_msgpack();
	bench_json();

	return 0;
}


