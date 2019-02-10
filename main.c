#include <curl/curl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <jansson.h>

#define ACCESS_TOKEN_HEADER "Authorization: Bearer YOUR_ACCESS_TOKEN"
/// Default path shouldnt contain slash for linux/unix or backslash for windows on end of the string
#define DEFAULT_PATH "/home/arslan/Desktop"

struct string {
    char *ptr;
    size_t len;
};

void init_string(struct string *s) {
    s->len = 0;
    s->ptr = (char *)malloc(s->len+1);
    if (s->ptr == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
    size_t new_len = s->len + size*nmemb;
    s->ptr = (char *)realloc(s->ptr, new_len+1);
    if (s->ptr == NULL) {
        fprintf(stderr, "realloc() failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(s->ptr+s->len, ptr, size*nmemb);
    s->ptr[new_len] = '\0';
    s->len = new_len;

    return size*nmemb;
}

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

char* get_list() {
    struct string s;
    init_string(&s);
    CURL *hnd = curl_easy_init();

    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(hnd, CURLOPT_URL, "https://api.dropboxapi.com/2/files/list_folder");

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, ACCESS_TOKEN_HEADER);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &s);
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, "{\n\"path\": \"\",\n\"recursive\": true,\n\"include_media_info\": false,\n\"include_deleted\": false,\n\"include_has_explicit_shared_members\": false,\n\"include_mounted_folders\": true\n}");

    CURLcode ret = curl_easy_perform(hnd);
    return s.ptr;
}

void download(const char *filePath,const char *fileName) {
    CURL *hnd = curl_easy_init();
    if(!hnd) return;

    FILE *fp;
    if (fp = fopen(fileName, "r")){
        printf("File exist: %s\n",fileName);
        fclose(fp);
        return;
    }
    fp = fopen(fileName,"wb");
    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(hnd, CURLOPT_URL, "https://content.dropboxapi.com/2/files/download");

    struct curl_slist *headers = NULL;
    /// Copy filepath
    char *dropboxApiArg = (char *) malloc(256 * sizeof(char));
    strcpy(dropboxApiArg,"Dropbox-API-Arg: {\"path\": \"");
    strcat(dropboxApiArg,filePath);
    strcat(dropboxApiArg,"\"}");

    headers = curl_slist_append(headers, "Content-Type: text/plain");
    headers = curl_slist_append(headers, dropboxApiArg);
    headers = curl_slist_append(headers, ACCESS_TOKEN_HEADER);
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
    CURLcode ret = curl_easy_perform(hnd);
    if(ret == CURLE_OK) {
        printf("Downloaded file: %s\n",fileName);
    }
    else {
        printf("An error occurared when downloading file: %s\n",fileName);
    }

    free(dropboxApiArg);
    fclose(fp);
}

int main(int argc, char **argv) {
    char *text = get_list();
    json_error_t error;
    json_t *root;
    root = json_loads(text, 0, &error);
    free(text); // we dont need json text anymore

    if(!root) {
        printf("Json load error!\n");
        return 0;
    }

    if(!json_is_object(root))
    {
        printf("error: root is not an object\n");
        json_decref(root);
        return 1;
    }

    json_t *entries = json_object_get(root,"entries");
    if(!entries) {
        printf("entries null\n");
        return 1;
    }

    const char *path = DEFAULT_PATH;
    for(int i = 0; i< json_array_size(entries); i++){
        const char *strPath, *strName;
        json_t *entry;
        long long int size = 0;

        entry = json_array_get(entries, i);
        if( NULL==entry ) continue;
        // get remote paths
        json_t *js_path = json_object_get(entry,"path_display");
        if(!js_path) continue;
        strPath = json_string_value(js_path);

        /// get file names
        json_t *js_name = json_object_get(entry,"name");
        if(!js_name) continue;
        strName = json_string_value(js_name);

        /// get file sizes
        json_t *js_size = json_object_get(entry,"size");
        if(js_size != NULL) {
            size = json_integer_value(js_size);
            // Download the file
            char *fullPath = (char *)malloc(256 * sizeof(char));
            strcpy(fullPath,path);
            strcat(fullPath,strPath);
            download(strPath,fullPath);
            free(fullPath);
        }
        else {
            // Create folder
            char *command = (char *)malloc(256 * sizeof(char));
            strcpy(command,"mkdir -p ");
            strcat(command,path);
            strcat(command,strPath);
            printf("Folder created %s\n",command);
            system(command);
            free(command);
        }

        //printf("%s %s %llu\n", strPath,strName, size);
    }
    json_decref(root);

    return 0;
}