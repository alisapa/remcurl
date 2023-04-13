#include <curl/curl.h>
#include <cjson/cJSON.h>

#define ID_LENGTH 50
static const char *rem_addr = "http://10.11.99.1";
static const char *list_url = "%s/documents/%s";
static const char *get_url  = "%s/download/%s/placeholder";
static const char *put_url  = "%s/upload";

struct buffer {
  char *ptr;
  size_t size;
};

size_t write_to_mem(void *contents, size_t size, size_t nmemb, void *userp);
size_t write_to_file(void *contents, size_t size, size_t nmemb, void *userp);
size_t write_discard(void *contents, size_t size, size_t nmemb, void *userp);
char *url_from_id(const char *pattern, char *id);
cJSON *get_request_json(CURL *handle, struct buffer *recvbuf, char *url);
int download_file(CURL *handle, char *file_id, char *out_base);
int upload_file(CURL *handle, char *path);
