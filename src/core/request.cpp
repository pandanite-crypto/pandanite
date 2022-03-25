#include "request.hpp"
#include <thread>
#include <emscripten/fetch.h>
using namespace std;
/*
    TODO: these will send call a javascript function which will block until the request completes
    and will send back the data
*/

void downloadSucceeded(emscripten_fetch_t *fetch) {
	printf("Finished downloading %llu bytes from URL %s.\n", fetch->numBytes, fetch->url);
	string buf(fetch->data, fetch->numBytes);
	cout<<"Received buffer: " << buf <<endl;
	emscripten_fetch_close(fetch);
}

void downloadFailed(emscripten_fetch_t *fetch) {
	emscripten_fetch_close(fetch);
}

vector<uint8_t> sendGetRequest(string url, uint32_t timeout) {
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.timeoutMSecs = timeout;
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_WAITABLE;
    attr.onsuccess = downloadSucceeded;
    attr.onerror = downloadFailed;
    emscripten_fetch_t * fetch = emscripten_fetch(&attr, url.c_str());
	emscripten_fetch_wait(fetch, timeout);
	string buf(fetch->data, fetch->numBytes);
	cout<<"Received buffer: " << buf <<endl;
    return vector<uint8_t>(buf.begin(), buf.end());
}
vector<uint8_t> sendPostRequest(string url, uint32_t timeout, vector<uint8_t>& content) {
    // emscripten_fetch_attr_t attr;
    // emscripten_fetch_attr_init(&attr);
    // strcpy(attr.requestMethod, "POST");
    // attr.timeoutMSecs = timeout;
    // attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    // attr.onsuccess = downloadSucceeded;
    // attr.onerror = downloadFailed;
    // attr.requestData = content.data();
    // attr.requestDataSize = content.size();

    // emscripten_fetch_t * fetch = emscripten_fetch(&attr, url.c_str());

    return vector<uint8_t>();
}
