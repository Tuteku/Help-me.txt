#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

extern long float_to_int(float value);
extern long add_offset(long value, long offset);

#define URL "https://api.worldbank.org/v2/country/AR/indicator/SI.POV.GINI?format=json&date=2011:2020&per_page=100"

typedef struct { char *data; size_t size; } Buffer;

static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    Buffer *buf = (Buffer *)userp;
    buf->data = realloc(buf->data, buf->size + total + 1);
    memcpy(buf->data + buf->size, ptr, total);
    buf->size += total;
    buf->data[buf->size] = '\0';
    return total;
}

int main(void) {
    Buffer buf = {NULL, 0};

    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, URL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    printf("%-6s  %-8s  %-10s  %-10s\n", "Anio", "GINI", "float_to_int", "add_offset");

    char *p = buf.data;
    float prev = 0;
    while ((p = strstr(p, "\"date\":\""))) {
        int year; float gini;
        if (sscanf(p, "\"date\":\"%d\"", &year) != 1) { p++; continue; }

        char *vp = strstr(p, "\"value\":");
        if (!vp) { p++; continue; }
        vp += 8;
        if (strncmp(vp, "null", 4) == 0) { p++; continue; }
        if (sscanf(vp, "%f", &gini) != 1) { p++; continue; }

        long as_int = float_to_int(gini);
        long with_offset = add_offset(as_int, 1);
        printf("%-6d  %-8.2f  %-12ld  %-10ld\n", year, gini, as_int, with_offset);

        p = vp;
    }

    free(buf.data);
    return 0;
}
