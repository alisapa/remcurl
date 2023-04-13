#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include <cjson/cJSON.h>

#include "download_funcs.h"

size_t write_to_mem(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t real_size = size * nmemb;
  struct buffer *buf = userp;
  char *ptr = realloc(buf->ptr, buf->size + real_size + 1);
  if (!ptr) {
    fprintf(stderr, "Failed to alloc memory for received data\n");
    return 0;
  }
  buf->ptr = ptr;
  memcpy(buf->ptr + buf->size, contents, real_size);
  buf->size += real_size;
  buf->ptr[buf->size] = 0; // Null-terminate the string
  return real_size;
}

size_t write_to_file(void *contents, size_t size, size_t nmemb, void *userp) {
  return fwrite(contents, size, nmemb, (FILE *)userp);
}

size_t write_discard(void *contents, size_t size, size_t nmemb, void *userp) {
  return size * nmemb; // Pretend we have successfully written it, while
                       // discarding all of the data
}

char *url_from_id(const char *pattern, char *id) {
  int strsiz = strlen(rem_addr) + strlen(list_url) + ID_LENGTH + 3;
  char *str = malloc(strsiz);
  if (!str) return NULL;

  snprintf(str, strsiz, pattern, rem_addr, id ? id : "");
  return str;
}

cJSON *get_request_json(CURL *handle, struct buffer *recvbuf, char *url) {
  curl_easy_setopt(handle, CURLOPT_HTTPGET, 1L);
  curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)recvbuf);
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_to_mem);
  curl_easy_setopt(handle, CURLOPT_URL, url);
  recvbuf->size = 0; // Reset receiving buffer
  CURLcode res = curl_easy_perform(handle);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
        curl_easy_strerror(res));
    return NULL;
  }
  
  cJSON *json = cJSON_ParseWithLength(recvbuf->ptr, recvbuf->size);
  if (!json) {
    fprintf(stderr, "Missing or malformed JSON.\n");
    return NULL;
  }
  return json;
}

int download_file(CURL *handle, char *file_id, char *out_base) {
  char *url = NULL;
  FILE *fp = NULL;

  url = url_from_id(get_url, file_id);
  if (!url) {
    fprintf(stderr, "Failed to allocate memory.\n");
    return 1;
  }

  char out_name[strlen(out_base) + 4];
  sprintf(out_name, "%s.pdf", out_base);
  fp = fopen(out_name, "w");
  if (!fp) {
    fprintf(stderr, "Failed to open for writing: %s\n", out_name);
    goto cleanup;
  }

  curl_easy_setopt(handle, CURLOPT_HTTPGET, 1L);
  curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)fp);
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_to_file);
  curl_easy_setopt(handle, CURLOPT_URL, url);
  CURLcode res = curl_easy_perform(handle);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
        curl_easy_strerror(res));
    goto cleanup;
  }
  fclose(fp);
  free(url);
  return 0;

cleanup:
  if (fp) fclose(fp);
  if (url) free(url);
  return 1;
}

int upload_file(CURL *handle, char *path) {
  CURLcode res;
  curl_mime *form = NULL;
  curl_mimepart *field = NULL;
  struct curl_slist *headerlist = NULL;
  const char *buf = "Expect:";

  char url[strlen(rem_addr) + strlen(put_url) + 1];
  sprintf(url, put_url, rem_addr);

  form = curl_mime_init(handle);
  field = curl_mime_addpart(form);
  curl_mime_name(field, "file");
  curl_mime_filedata(field, path);

  headerlist = curl_slist_append(headerlist, buf);
  curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headerlist);
  curl_easy_setopt(handle, CURLOPT_MIMEPOST, form);
  curl_easy_setopt(handle, CURLOPT_URL, url);

  // Don't print response such as:
  //   {"status":"Upload successful"}
  //
  // TODO: Should use response for error-checking instead?
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_discard);

  res = curl_easy_perform(handle);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
        curl_easy_strerror(res));
    curl_mime_free(form);
    curl_slist_free_all(headerlist);
    return 1;
  }

  curl_mime_free(form);
  curl_slist_free_all(headerlist);
  return 0;
}
