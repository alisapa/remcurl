#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include <curl/curl.h>
#include <cjson/cJSON.h>

#include "download_funcs.h"

#define STREQ(arg,str) (strncmp(arg, str, strlen(str) + 1) == 0)

// Not a typo! Or rather, the Remarkable developers' typo...
const char *name_attr      = "VissibleName";
const char *id_attr        = "ID";
const char *type_attr      = "Type";

// Two zero-terminated strings in the same buffer
struct splitstr {
  char *str;
  char *str2; // Pointing to the same malloc() as str
};
struct buffer recvbuf = { 0 };
CURL *handle = NULL;

void ZERO_OR_DIE(int ret, char *msg) {
  if (ret != 0) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
  }
}

void NZP_OR_DIE(void *ret, char *msg) {
  if (ret == NULL) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
  }
}

char *get_string_attr(cJSON *object, const char *name) {
  for (cJSON *attr = object->child; attr; attr = attr->next) {
    if (!cJSON_IsString(attr)) continue;
    if (strcmp(attr->string, name) == 0)
      return attr->valuestring;
  }
  return NULL;
}

int is_directory(cJSON *item) {
  char *type = get_string_attr(item, type_attr);
  if (!type) return 1; // The base path, "", doesn't have a type and is a
                       // directory.
  return strcmp(type, "CollectionType") == 0;
}

cJSON *find_name_in_directory(cJSON *dir_json, char *find_name) {
  for (cJSON *item = dir_json->child; item; item = item->next) {
    char *name = get_string_attr(item, name_attr);
    if (name && strcmp(name, find_name) == 0) {
      // We must make a copy of the item's JSON, so that it persists
      // when the directory JSON is deallocated.
      cJSON *item_copy = cJSON_Duplicate(item, 1);
      if (!item_copy)
        fprintf(stderr, "Failed to alloc memory.\n");
      return item_copy;
    }
  }
  fprintf(stderr, "Not found: %s\n", find_name);
  return NULL;
}

cJSON *get_json_from_filename(char *dir_id, char *find_name) {
  cJSON *json, *entry;
  char *id;

  char *url = url_from_id(list_url, dir_id);
  json = get_request_json(handle, &recvbuf, url);
  free(url);
  if (!json) return NULL;
  if (!json->child) {
    fprintf(stderr, "Directory is empty.\n");
    cJSON_Delete(json);
    return NULL;
  }

  cJSON *result = find_name_in_directory(json, find_name);
  cJSON_Delete(json);
  return result;
}

int split_path(char *str, struct splitstr *dest) {
  dest->str = malloc(strlen(str) + 2);
  if (!dest->str) {
    fprintf(stderr, "Failed to allocate memory.\n");
    return 1;
  }

  for (int i = str[0] != 0 ? strlen(str) - 1 : 0; ; i--) {
    if (str[i] == '/') {
      strcpy(dest->str, str);
      dest->str[i] = 0;
      dest->str2 = dest->str + i + 1;
      break;
    }
    if (str[i] == 0) {
      dest->str[0] = 0;
      strcpy(dest->str + 1, str);
      break;
    }
  }
  return 0;
}

cJSON *get_json_from_path(char *pth) {
  // Make a copy of the path, because we'll be modifying it
  char tmp[strlen(pth) + 1];
  char *path = tmp; // Otherwise C compiler complains about array rather than
                    // pointer
  strcpy(path, pth);

  // Get root path
  // TODO: Make IP adjustable at compile time
  cJSON *json = get_request_json(handle, &recvbuf,
      "http://10.11.99.1/documents/");
  cJSON *new_json = json;
  if (!json) {
    fprintf(stderr, "Failed to alloc memory.\n");
    return NULL;
  }

  size_t next_slash = -1;
  do {
    if (!next_slash) break;
    path += next_slash + 1;
    char *p;
    for (p = path; *p && *p != '/'; p++);
    if (*p) {
      next_slash = p - path;
      *p = 0;
    } else {
      next_slash = 0;
    }
    if (!*path) break;

    // If this was the final item of the path, we broke out of the loop in the
    // above section. The following code is executed for all path items
    // except for the last.
    if (!is_directory(json)) {
      fprintf(stderr, "Not a directory: %s\n", path);
      cJSON_Delete(new_json);
      return NULL;
    }
    new_json = get_json_from_filename(get_string_attr(json, id_attr), path);
    cJSON_Delete(json);
    if (!new_json) return NULL;
    json = new_json;
  } while (1);
  return new_json;
}

int list_path(char *path) {
  char *url = NULL;
  cJSON *list_json = NULL; // Information about the contents of the directory
  cJSON *dir_json = get_json_from_path(path); // Information about the
                                                       // directory itself
  if (!dir_json) goto cleanup;
  if (!is_directory(dir_json)) {
    fprintf(stderr, "Not a directory: %s\n", path);
    goto cleanup;
  }

  url = url_from_id(list_url, get_string_attr(dir_json, id_attr));
  list_json = get_request_json(handle, &recvbuf, url);
  free(url);
  if (!list_json) goto cleanup;

  for (cJSON *item = list_json->child; item; item = item->next) {
    char *name = get_string_attr(item, name_attr);
    if (name)
      printf("%s%s\n", name, is_directory(item) ? "/" : "");
  }

  if (list_json) cJSON_Delete(list_json);
  if (dir_json) cJSON_Delete(dir_json);
  return 0;

cleanup:
  if (list_json) cJSON_Delete(list_json);
  if (dir_json) cJSON_Delete(dir_json);
  return 1;
}

int recursive_download(cJSON *start_json) {
  char *dir_name = get_string_attr(start_json, name_attr);
  dir_name = dir_name ? dir_name : "fs_root";
  if (mkdir(dir_name, 0755) && errno != EEXIST) {
    perror("Failed to create directory");
    return 1;
  }
  if (chdir(dir_name)) {
    perror("Failed to change into directory");
    return 1;
  }

  char *start_id = get_string_attr(start_json, id_attr);
  start_id = start_id ? start_id : "";
  char *url = url_from_id(list_url, start_id);
  if (!url) return 1;
  cJSON *json = get_request_json(handle, &recvbuf, url);
  free(url);
  if (!json) return 1;

  for (cJSON *item = json->child; item; item = item->next) {
    if (is_directory(item)) {
      printf("Saving directory: %s\n", get_string_attr(item, name_attr));
      recursive_download(item);
      continue;
    }
    printf("Saving file: %s\n", get_string_attr(item, name_attr));
    download_file(handle, get_string_attr(item, id_attr),
                  get_string_attr(item, name_attr));
  }
  cJSON_Delete(json);
  if (chdir("..")) {
    perror("Failed to change to parent directory");
    return 1;
  }
  return 0;
}

int save_file_from_path(char *path) {
  if (! *path) { // to avoid needing to handle this edge case
                 // in the further code
    struct cJSON root_cjson = {0};
    return recursive_download(&root_cjson);
  }

  cJSON *file_info = get_json_from_path(path);
  if (!file_info) return 1;
  if (is_directory(file_info)) {
    if (recursive_download(file_info))
      goto cleanup;
  } else {
    if (download_file(handle, get_string_attr(file_info, id_attr),
          get_string_attr(file_info, name_attr)))
      goto cleanup;
  }
  return 0;

cleanup:
  if (file_info) cJSON_Delete(file_info);
  return 1;
}

int print_json_from_path(char *path) {
  cJSON *json = get_json_from_path(path);
  if (!json) return 1;
  char *json_str = cJSON_Print(json);
  printf("%s\n", json_str);
  free(json_str);
  cJSON_Delete(json);
  return 0;
}

int main(int argc, char **argv) {
  CURLcode res;
  int ret = 0;

  ZERO_OR_DIE(curl_global_init(CURL_GLOBAL_DEFAULT), "Failed to init libcurl.");
  NZP_OR_DIE(handle = curl_easy_init(), "Failed to init CURL easy handle.");
  NZP_OR_DIE(recvbuf.ptr = malloc(1024), "Failed to alloc memory.");
  recvbuf.size = 0; // No data currently in the buffer

  if (argc < 2 || STREQ(argv[1], "ls")) {
    ret = list_path(argc > 2 ? argv[2] : "");
  } else if (STREQ(argv[1], "get")) {
    if (argc < 3) {
      fprintf(stderr, "Missing file path.\n");
      goto cleanup;
    }
    ret = save_file_from_path(argv[2]);
  } else if (STREQ(argv[1], "put")) {
    if (argc < 3) {
      fprintf(stderr, "Missing file path.\n");
      goto cleanup;
    }
    ret = upload_file(handle, argv[2]);
  } else if (STREQ(argv[1], "json")) {
    if (argc < 3) {
      fprintf(stderr, "Missing file path.\n");
      goto cleanup;
    }
    ret = print_json_from_path(argv[2]);
  } else {
    fprintf(stderr, "Unrecognized action: %s\n", argv[1]);
    ret = 1;
  }

cleanup:
  free(recvbuf.ptr);
  curl_easy_cleanup(handle);
  curl_global_cleanup();
  return ret;
}
