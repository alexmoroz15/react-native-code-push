#include "pch.h"

#include "CodePushDownloadHandler.h"

using namespace CodePush;

CodePushDownloadHandler::CodePushDownloadHandler(
	path downloadFilePath,
	function<void(int64_t, int64_t)> progressCallback,
	function<void(bool)> doneCallback,
	function<void(const hresult_error&)> failCallback)
{

}

/*
- (id)init:(NSString *)downloadFilePath
operationQueue:(dispatch_queue_t)operationQueue
progressCallback:(void (^)(long long, long long))progressCallback
doneCallback:(void (^)(BOOL))doneCallback
failCallback:(void (^)(NSError *err))failCallback {
    self.outputFileStream = [NSOutputStream outputStreamToFileAtPath:downloadFilePath
                                                              append:NO];
    self.receivedContentLength = 0;
    self.operationQueue = operationQueue;
    self.progressCallback = progressCallback;
    self.doneCallback = doneCallback;
    self.failCallback = failCallback;
    return self;
}

- (void)download:(NSString *)url {
    self.downloadUrl = url;
    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:url]
                                             cachePolicy:NSURLRequestUseProtocolCachePolicy
                                         timeoutInterval:60.0];
    NSURLConnection *connection = [[NSURLConnection alloc] initWithRequest:request
                                                                  delegate:self
                                                          startImmediately:NO];
    if ([NSOperationQueue instancesRespondToSelector:@selector(setUnderlyingQueue:)]) {
        NSOperationQueue *delegateQueue = [NSOperationQueue new];
        delegateQueue.underlyingQueue = self.operationQueue;
        [connection setDelegateQueue:delegateQueue];
    } else {
        [connection scheduleInRunLoop:[NSRunLoop mainRunLoop]
                              forMode:NSDefaultRunLoopMode];
    }

    [connection start];
}
*/

void CodePushDownloadHandler::Download(wstring_view url)
{

}