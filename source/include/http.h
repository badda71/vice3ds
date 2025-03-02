#pragma once

#define R_APP_INVALID_ARGUMENT MAKERESULT(RL_PERMANENT, RS_INVALIDARG, RM_APPLICATION, 1)
#define R_APP_FILEOPEN_FAILED MAKERESULT(RL_PERMANENT, RS_CANCELED, RM_APPLICATION, 2)
#define R_APP_SKIPPED MAKERESULT(RL_PERMANENT, RS_NOTSUPPORTED, RM_APPLICATION, 3)

#define R_APP_THREAD_CREATE_FAILED MAKERESULT(RL_PERMANENT, RS_INTERNAL, RM_APPLICATION, 4)

#define R_APP_PARSE_FAILED MAKERESULT(RL_PERMANENT, RS_INTERNAL, RM_APPLICATION, 5)
#define R_APP_BAD_DATA MAKERESULT(RL_PERMANENT, RS_INTERNAL, RM_APPLICATION, 6)

#define R_APP_HTTP_TOO_MANY_REDIRECTS MAKERESULT(RL_PERMANENT, RS_INTERNAL, RM_APPLICATION, 7)
#define R_APP_HTTP_ERROR_BASE MAKERESULT(RL_PERMANENT, RS_INTERNAL, RM_APPLICATION, 8)

#define R_APP_HTTP_ERROR_END (R_APP_HTTP_ERROR_BASE + 600)
#define R_APP_CURL_INIT_FAILED R_APP_HTTP_ERROR_END
#define R_APP_CURL_ERROR_BASE (R_APP_CURL_INIT_FAILED + 1)
#define R_APP_CURL_ERROR_END (R_APP_CURL_ERROR_BASE + 100)

#define R_APP_NOT_IMPLEMENTED MAKERESULT(RL_PERMANENT, RS_INTERNAL, RM_APPLICATION, RD_NOT_IMPLEMENTED)
#define R_APP_OUT_OF_MEMORY MAKERESULT(RL_FATAL, RS_OUTOFRESOURCE, RM_APPLICATION, RD_OUT_OF_MEMORY)
#define R_APP_OUT_OF_RANGE MAKERESULT(RL_PERMANENT, RS_INVALIDARG, RM_APPLICATION, RD_OUT_OF_RANGE)

#define HTTP_ERRBUFSIZE 256

typedef struct {
	long mtime;
} http_info;

extern http_info http_last_req_info;
extern char http_errbuf[HTTP_ERRBUFSIZE];

Result http_download_buffer(const char* url, u32* downloadedSize, void* buf, size_t size);
Result http_download_file(
	const char* url,
	const char *fname,
	Result (*checkRunning)(),
	Result (*progress)(u64 total, u64 curr));
Result http_check_url(const char* url);

extern unsigned int http_bufsize;
