#include "request.hpp"
#include <thread>
#include <emscripten/fetch.h>
using namespace std;
/*
    TODO: these will send call a javascript function which will block until the request completes
    and will send back the data
*/


vector<uint8_t> sendGetRequest(string url, uint32_t timeout) {
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS| EMSCRIPTEN_FETCH_REPLACE ;
    emscripten_fetch_t * fetch = emscripten_fetch(&attr, url.c_str());
	
	EMSCRIPTEN_RESULT ret = EMSCRIPTEN_RESULT_TIMED_OUT;
  	while(ret == EMSCRIPTEN_RESULT_TIMED_OUT) {
		ret = emscripten_fetch_wait(fetch, 0);
	}
	if (fetch->status == 200) {
		printf("Finished downloading %llu bytes from URL %s.\n", fetch->numBytes, fetch->url);
		// The data is now available at fetch->data[0] through fetch->data[fetch->numBytes-1];
	} else {
		printf("Downloading %s failed, HTTP failure status code: %d.\n", fetch->url, fetch->status);
	}
	cout<<fetch->numBytes<<endl;
	string buf(fetch->data, fetch->numBytes);
	cout<<"Received buffer: " << buf <<endl;
	emscripten_fetch_close(fetch);
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
