#pragma once
#include <string>
#include <curl/curl.h>

struct HttpResult {
    bool ok;
    int statusCode;
    std::string body;
    std::string error;
};

namespace HttpDetail {

static size_t WriteCallback(void* ptr, size_t size, size_t nmemb, std::string* out) {
    out->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

static HttpResult Perform(const std::string& method, const std::string& url,
                          const std::string& body = "", const std::string& contentType = "application/json") {
    CURL* curl = curl_easy_init();
    if (!curl) return {false, 0, "", "Failed to initialize HTTP client"};

    std::string responseBody;
    struct curl_slist* headers = nullptr;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    if (method == "POST" || method == "PUT" || method == "PATCH") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body.size());
        if (!contentType.empty()) {
            std::string header = "Content-Type: " + contentType;
            headers = curl_slist_append(headers, header.c_str());
        }
    } else if (method == "DELETE") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    }

    if (headers) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        return {false, 0, "", std::string(curl_easy_strerror(res))};
    }

    bool success = httpCode >= 200 && httpCode < 300;
    return {success, (int)httpCode, responseBody,
            success ? "" : "HTTP " + std::to_string(httpCode) + ": " + responseBody};
}

}

inline HttpResult HttpGet(const std::string& url) {
    return HttpDetail::Perform("GET", url);
}

inline HttpResult HttpPost(const std::string& url, const std::string& body,
                           const std::string& contentType = "application/json") {
    return HttpDetail::Perform("POST", url, body, contentType);
}

inline HttpResult HttpPut(const std::string& url, const std::string& body,
                          const std::string& contentType = "application/json") {
    return HttpDetail::Perform("PUT", url, body, contentType);
}

inline HttpResult HttpDelete(const std::string& url) {
    return HttpDetail::Perform("DELETE", url);
}
