#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <time.h>

#include <3ds.h>
#include <curl/curl.h>
#include <zlib.h>

#include "http.h"
#include "archdep_cp.h"
#include "log.h"
#include "vice3ds.h"
#include "archdep.h"

#define HTTP_USER_AGENT "Mozilla/5.0 (Nintendo 3DS; Mobile; rv:10.0) Gecko/20100101 vice3DS"

#define HTTP_MAX_REDIRECTS 50
#define HTTP_TIMEOUT_SEC 15
#define HTTP_TIMEOUT_NS ((u64) HTTP_TIMEOUT_SEC * 1000000000)

char http_errbuf[HTTP_ERRBUFSIZE];
http_info http_last_req_info = {0};
unsigned int http_bufsize = 0;

struct httpc_context_s {
    httpcContext httpc;

    bool compressed;
    z_stream inflate;
    u8 buffer[32 * 1024];
    u32 bufferSize;
};

typedef struct httpc_context_s* httpc_context;

static void string_copy(char* dst, const char* src, size_t size) {
    if(size > 0) {
        strncpy(dst, src, size - 1);
        dst[size - 1] = '\0';
    }
}

static void httpc_resolve_redirect(char* oldUrl, const char* redirectTo, size_t size) {
    if(size > 0) {
        if(redirectTo[0] == '/') {
            char* baseEnd = oldUrl;

            // Find the third slash to find the end of the URL's base; e.g. https://www.example.com/
            u32 slashCount = 0;
            while(*baseEnd != '\0' && (baseEnd = strchr(baseEnd + 1, '/')) != NULL) {
                slashCount++;
                if(slashCount == 3) {
                    break;
                }
            }

            // If there are less than 3 slashes, assume the base URL ends at the end of the string; e.g. https://www.example.com
            if(slashCount != 3) {
                baseEnd = oldUrl + strlen(oldUrl);
            }

            size_t baseLen = baseEnd - oldUrl;
            if(baseLen < size) {
                string_copy(baseEnd, redirectTo, size - baseLen);
            }
        } else {
            string_copy(oldUrl, redirectTo, size);
        }
    }
}

static Result httpc_open(httpc_context* context, const char* url, bool userAgent, HTTPC_RequestMethod method) {
    if(url == NULL) {
		snprintf(http_errbuf,HTTP_ERRBUFSIZE,"httpc open: url is NULL");
		return R_APP_INVALID_ARGUMENT;
    }

    Result res = 0;

    httpc_context ctx = (httpc_context) calloc(1, sizeof(struct httpc_context_s));
    if(ctx != NULL) {
        char currUrl[1024];
        string_copy(currUrl, url, sizeof(currUrl));

        bool resolved = false;
        u32 redirectCount = 0;
        while(R_SUCCEEDED(res) && !resolved && redirectCount < HTTP_MAX_REDIRECTS) {
            if(R_SUCCEEDED(res = httpcOpenContext(&ctx->httpc, method, currUrl, 1))) {
                u32 response = 0;
                if(R_SUCCEEDED(res = httpcSetSSLOpt(&ctx->httpc, SSLCOPT_DisableVerify))
                   && (!userAgent || R_SUCCEEDED(res = httpcAddRequestHeaderField(&ctx->httpc, "User-Agent", HTTP_USER_AGENT)))
                   && R_SUCCEEDED(res = httpcAddRequestHeaderField(&ctx->httpc, "Accept-Encoding", "gzip, deflate"))
                   && R_SUCCEEDED(res = httpcSetKeepAlive(&ctx->httpc, HTTPC_KEEPALIVE_ENABLED))
                   && R_SUCCEEDED(res = httpcBeginRequest(&ctx->httpc))
                   && R_SUCCEEDED(res = httpcGetResponseStatusCodeTimeout(&ctx->httpc, &response, HTTP_TIMEOUT_NS))) {
                    if(response == 301 || response == 302 || response == 303) {
                        redirectCount++;

                        char redirectTo[1024];
                        memset(redirectTo, '\0', sizeof(redirectTo));
                        if(R_SUCCEEDED(res = httpcGetResponseHeader(&ctx->httpc, "Location", redirectTo, sizeof(redirectTo)))) {
                            httpcCloseContext(&ctx->httpc);

                            httpc_resolve_redirect(currUrl, redirectTo, sizeof(currUrl));
                        }
                    } else {
                        resolved = true;

                        if(response == 200) {
                            char encoding[32];
                            if(R_SUCCEEDED(httpcGetResponseHeader(&ctx->httpc, "Content-Encoding", encoding, sizeof(encoding)))) {
                                bool gzip = strncmp(encoding, "gzip", sizeof(encoding)) == 0;
                                bool deflate = strncmp(encoding, "deflate", sizeof(encoding)) == 0;

                                ctx->compressed = gzip || deflate;

                                if(ctx->compressed) {
                                    memset(&ctx->inflate, 0, sizeof(ctx->inflate));
                                    if(deflate) {
                                        inflateInit(&ctx->inflate);
                                    } else if(gzip) {
                                        inflateInit2(&ctx->inflate, MAX_WBITS | 16);
                                    }
                                }
                            }
                        } else {
                      		snprintf(http_errbuf,HTTP_ERRBUFSIZE,"httpc: HTTP error code %ld", response);
							res = R_APP_HTTP_ERROR_BASE + response;
                        }
                    }
                }

                if(R_FAILED(res)) {
                    httpcCloseContext(&ctx->httpc);
                }
            }
        }

        if(R_SUCCEEDED(res) && redirectCount >= 32) {
            snprintf(http_errbuf,HTTP_ERRBUFSIZE,"httpc: too many redirects");
            res = R_APP_HTTP_TOO_MANY_REDIRECTS;
        }

        if(R_FAILED(res)) {
            free(ctx);
        }
    } else {
        snprintf(http_errbuf,HTTP_ERRBUFSIZE,"httpc: could not allocate http context");
        res = R_APP_OUT_OF_MEMORY;
    }

    if(R_SUCCEEDED(res)) {
        *context = ctx;
    }

    return res;
}

static Result httpc_close(httpc_context context) {
    if(context == NULL) {
		snprintf(http_errbuf,HTTP_ERRBUFSIZE,"httpc close: context is NULL");
        return R_APP_INVALID_ARGUMENT;
    }

    if(context->compressed) {
        inflateEnd(&context->inflate);
    }

    Result res = httpcCloseContext(&context->httpc);
    free(context);
    return res;
}

static Result httpc_get_size(httpc_context context, u32* size) {
    if(context == NULL || size == NULL) {
		snprintf(http_errbuf,HTTP_ERRBUFSIZE,"httpc get size: context or size is NULL");
        return R_APP_INVALID_ARGUMENT;
    }

    return httpcGetDownloadSizeState(&context->httpc, NULL, size);
}

static Result httpc_read(httpc_context context, u32* bytesRead, void* buffer, u32 size) {
    if(context == NULL || buffer == NULL) {
		snprintf(http_errbuf,HTTP_ERRBUFSIZE,"httpc: context or buffer is NULL");
        return R_APP_INVALID_ARGUMENT;
    }

    Result res = 0;

    u32 startPos = 0;
    if(R_SUCCEEDED(res = httpcGetDownloadSizeState(&context->httpc, &startPos, NULL))) {
        res = HTTPC_RESULTCODE_DOWNLOADPENDING;

        u32 outPos = 0;
        if(context->compressed) {
            u32 lastPos = context->bufferSize;
            while(res == HTTPC_RESULTCODE_DOWNLOADPENDING && outPos < size) {
                if((context->bufferSize > 0
                    || R_SUCCEEDED(res = httpcReceiveDataTimeout(&context->httpc, &context->buffer[context->bufferSize], sizeof(context->buffer) - context->bufferSize, HTTP_TIMEOUT_NS))
                    || res == HTTPC_RESULTCODE_DOWNLOADPENDING)) {
                    Result posRes = 0;
                    u32 currPos = 0;
                    if(R_SUCCEEDED(posRes = httpcGetDownloadSizeState(&context->httpc, &currPos, NULL))) {
                        context->bufferSize += currPos - lastPos;

                        context->inflate.next_in = context->buffer;
                        context->inflate.next_out = buffer + outPos;
                        context->inflate.avail_in = context->bufferSize;
                        context->inflate.avail_out = size - outPos;
                        inflate(&context->inflate, Z_SYNC_FLUSH);

                        memcpy(context->buffer, context->buffer + (context->bufferSize - context->inflate.avail_in), context->inflate.avail_in);
                        context->bufferSize = context->inflate.avail_in;

                        lastPos = currPos;
                        outPos = size - context->inflate.avail_out;
                    } else {
                        res = posRes;
                    }
                }
            }
       } else {
            while(res == HTTPC_RESULTCODE_DOWNLOADPENDING && outPos < size) {
                if(R_SUCCEEDED(res = httpcReceiveDataTimeout(&context->httpc, &((u8*) buffer)[outPos], size - outPos, HTTP_TIMEOUT_NS)) || res == HTTPC_RESULTCODE_DOWNLOADPENDING) {
                    Result posRes = 0;
                    u32 currPos = 0;
                    if(R_SUCCEEDED(posRes = httpcGetDownloadSizeState(&context->httpc, &currPos, NULL))) {
                        outPos = currPos - startPos;
                    } else {
                        res = posRes;
                    }
                }
			}
        }

        if(res == HTTPC_RESULTCODE_DOWNLOADPENDING) {
            res = 0;
        }

        if(R_SUCCEEDED(res) && bytesRead != NULL) {
            *bytesRead = outPos;
        }
    }

    return res;
}

#define R_HTTP_TLS_VERIFY_FAILED 0xD8A0A03C

typedef struct {
    u32 bufferSize;
    void* userData;
    Result (*callback)(void* userData, void* buffer, size_t size);
    Result (*checkRunning)(void* userData);
    Result (*progress)(void* userData, u64 total, u64 curr);

    void* buf;
    u32 pos;

    Result res;
} http_curl_data;

static size_t http_curl_write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    http_curl_data* curlData = (http_curl_data*) userdata;

    size_t srcPos = 0;
    size_t available = size * nmemb;
    while(R_SUCCEEDED(curlData->res) && available > 0) {
        size_t remaining = curlData->bufferSize - curlData->pos;
        size_t copySize = available < remaining ? available : remaining;

        memcpy((u8*) curlData->buf + curlData->pos, ptr + srcPos, copySize);
        curlData->pos += copySize;

        srcPos += copySize;
        available -= copySize;

        if(curlData->pos == curlData->bufferSize) {
            curlData->res = curlData->callback(curlData->userData, curlData->buf, curlData->bufferSize);
            curlData->pos = 0;
        }
    }

    return R_SUCCEEDED(curlData->res) ? size * nmemb : 0;
}

static int http_curl_xfer_info_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    http_curl_data* curlData = (http_curl_data*) clientp;

    if(R_FAILED(curlData->res) || (curlData->checkRunning != NULL && R_FAILED(curlData->res = curlData->checkRunning(curlData->userData)))) {
        return 1;
    }

    if(curlData->progress != NULL) {
        curlData->progress(curlData->userData, (u64) dltotal, (u64) dlnow);
    }

    return 0;
}

static Result http_download_callback(	// )
	const char* url,
	u32 bufferSize,
	void* userData,
	Result (*callback)(void* userData, void* buffer, size_t size),
	Result (*checkRunning)(void* userData),
	Result (*progress)(void* userData, u64 total, u64 curr),
	int headonly)
{
	Result res = 0;

	static int isInit=0;
	*http_errbuf=0;

	if (!isInit) {
        if (archdep_network_init()) {
    		snprintf(http_errbuf,HTTP_ERRBUFSIZE,"network init failed");
			return -1;
		}
		httpcInit(0); // Buffer size when POST/PUT
		atexit(httpcExit);
		
		isInit=1;
	}

	http_last_req_info.mtime=-1;
    void* buf = malloc(bufferSize);
    if(buf != NULL) {
		
		// try (faster) httpc service if not using https
		httpc_context context = NULL;
        if(url[4] != 's' && R_SUCCEEDED(res = httpc_open(&context, url, true, headonly ? HTTPC_METHOD_HEAD : HTTPC_METHOD_GET))) {

			log_message(LOG_DEFAULT,"downloading with httpc %s",url);
			char modified[64];
			if(R_SUCCEEDED(res = httpcGetResponseHeader(&(context->httpc), "Last-Modified", modified, sizeof(modified)))) {
				http_last_req_info.mtime = curl_getdate(modified,NULL);
			}
			if (headonly) {
				httpc_close(context);		
			} else {
				u32 dlSize = 0;
				if(R_SUCCEEDED(res = httpc_get_size(context, &dlSize))) {
					if(progress != NULL) {
						progress(userData, dlSize, 0);
					}

					u32 total = 0;
					u32 currSize = 0;
					while(total < dlSize
						  && (checkRunning == NULL || R_SUCCEEDED(res = checkRunning(userData)))
						  && R_SUCCEEDED(res = httpc_read(context, &currSize, buf, bufferSize)))
					{
						if (R_FAILED(res = callback(userData, buf, currSize))) {
							snprintf(http_errbuf,HTTP_ERRBUFSIZE,"Aborted by user");
							break;
						}
						if(progress != NULL) {
	                        progress(userData, dlSize, total);
						}
						total += currSize;
					}

					Result closeRes = httpc_close(context);
					if(R_SUCCEEDED(res)) {
						res = closeRes;
					}
				}
            }

		// try curl if https (or httpc service was redircted to https)
        } else if (res == R_HTTP_TLS_VERIFY_FAILED || url[4] == 's') {
			res = 0;

			log_message(LOG_DEFAULT,"downloading with curl %s",url);
            CURL* curl = curl_easy_init();
            if(curl != NULL) {
                http_curl_data curlData = {bufferSize, userData, callback, checkRunning, progress, buf, 0, 0};

                curl_easy_setopt(curl, CURLOPT_URL, url);
//                curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, bufferSize);
                curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
                curl_easy_setopt(curl, CURLOPT_USERAGENT, HTTP_USER_AGENT);
                curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, (long) HTTP_TIMEOUT_SEC);
                curl_easy_setopt(curl, CURLOPT_MAXREDIRS, (long) HTTP_MAX_REDIRECTS);
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
//                curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long) CURL_HTTP_VERSION_2TLS);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_curl_write_callback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) &curlData);
                curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
                curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, http_curl_xfer_info_callback);
                curl_easy_setopt(curl, CURLOPT_XFERINFODATA, (void*) &curlData);
				curl_easy_setopt(curl, CURLOPT_FILETIME, 1L);
				//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
				if (headonly) {
					curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
				}

                CURLcode ret = curl_easy_perform(curl);

                if(ret == CURLE_OK)
					curl_easy_getinfo(curl, CURLINFO_FILETIME, &(http_last_req_info.mtime));

				if(ret == CURLE_OK && curlData.pos != 0) {
                    curlData.res = curlData.callback(curlData.userData, curlData.buf, curlData.pos);
                    curlData.pos = 0;
                }

                res = curlData.res;

                if(ret != CURLE_OK) {
                    if(ret == CURLE_HTTP_RETURNED_ERROR) {
                        long responseCode = 0;
                        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
                        res = R_APP_HTTP_ERROR_BASE + responseCode;
                   		snprintf(http_errbuf,HTTP_ERRBUFSIZE,"curl: HTTP error code %ld", responseCode);
                    } else {
						if (ret == CURLE_ABORTED_BY_CALLBACK)
							snprintf(http_errbuf,HTTP_ERRBUFSIZE,"Aborted by user");
						else
							snprintf(http_errbuf,HTTP_ERRBUFSIZE,"curl error: %s", curl_easy_strerror(ret));
                        res = R_APP_CURL_ERROR_BASE + ret;
                    }
                }

                curl_easy_cleanup(curl);
            } else {
				snprintf(http_errbuf,HTTP_ERRBUFSIZE,"curl init failed");
                res = R_APP_CURL_INIT_FAILED;
            }
		} else {
			snprintf(http_errbuf,HTTP_ERRBUFSIZE,"%s (%p)", result_translate(res), (void*)res);
		}

        free(buf);
    } else {
		snprintf(http_errbuf,HTTP_ERRBUFSIZE,"could not allocate download buffer");
        res = R_APP_OUT_OF_MEMORY;
    }
	if (res) log_message(LOG_DEFAULT,"download error: %s, %s",http_errbuf, url);
	return res;
}

typedef struct {
    void* buf;
    size_t size;

    size_t pos;
} http_buffer_data;

static Result http_download_buffer_callback(void* userData, void* buffer, size_t size) {
    http_buffer_data* data = (http_buffer_data*) userData;

    size_t remaining = data->size - data->pos;
    size_t copySize = size;
    if(copySize > remaining) {
        copySize = remaining;
    }

    if(copySize > 0) {
        memcpy((u8*) data->buf + data->pos, buffer, copySize);
        data->pos += copySize;
    }

    return 0;
}

Result http_download_buffer(const char* url, u32* downloadedSize, void* buf, size_t size) {
    http_buffer_data data = {buf, size, 0};
    Result res = http_download_callback(url, size, &data, http_download_buffer_callback, NULL, NULL, 0);
    return res;
}

Result http_check_url(const char* url) {
    return http_download_callback(url, 8 * 1024, NULL, NULL, NULL, NULL, 1);
}

typedef struct {
    FILE *fd;
	Result (*checkRunning)();
	Result (*progress)(u64 total, u64 curr);
} http_file_data;


static Result http_download_file_callback(void* userData, void* buffer, size_t size) {
	http_file_data* data = (http_file_data*) userData;
	int i = fwrite(buffer, 1, size, data->fd);
	if (i != size) return -1;
	return 0;
}

static Result http_download_file_checkRunning(void* userData) {
	http_file_data* data = (http_file_data*) userData;
	if (data->checkRunning) return data->checkRunning();
	return 0;
}

static Result http_download_file_progress (void* userData, u64 total, u64 curr) {
	http_file_data* data = (http_file_data*) userData;
	if (data->progress)	return data->progress(total, curr);
	return 0;
}

#define BUFFER_SIZE_N3DS 256 * 1024
#define BUFFER_SIZE_O3DS 32 * 1024

Result http_download_file( // )
	const char* url,
	const char *fname,
	Result (*checkRunning)(),
	Result (*progress)(u64 total, u64 curr))
{
	mkpath((char *)fname, 0);
	FILE *fd = fopen (fname, "wb");
	if (fd == NULL) {
		snprintf(http_errbuf,HTTP_ERRBUFSIZE,"could not open file %s for writing", fname);
		return R_APP_FILEOPEN_FAILED;
	}
    http_file_data data = {fd, checkRunning, progress};
	Result res = http_download_callback(url, http_bufsize > 0 ? http_bufsize : (isN3DS()?BUFFER_SIZE_N3DS:BUFFER_SIZE_O3DS), &data, http_download_file_callback, http_download_file_checkRunning, http_download_file_progress, 0);
	fclose(fd);
	if(R_FAILED(res)) {
		unlink(fname);
	}
	return res;
}
