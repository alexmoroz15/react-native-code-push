#include "pch.h"

#include "winrt/Windows.Data.Json.h"
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Storage.h"
#include "winrt/Windows.Storage.Streams.h"
#include "winrt/Windows.Web.Http.h"
#include "winrt/Windows.Web.Http.Headers.h"

#include "CodePushDownloadHandler.h"

using namespace CodePush;

using namespace winrt;
using namespace Windows::Data::Json;
using namespace Windows::Foundation;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::Web::Http;

CodePushDownloadHandler::CodePushDownloadHandler(
    StorageFile downloadFile,
    function<void(int64_t, int64_t)> progressCallback,
    function<void(bool)> doneCallback,
    function<void(const hresult_error&)> failCallback) :
    receivedContentLength(0),
    expectedContentLength(0),
    progressCallback(progressCallback),
    doneCallback(doneCallback),
    failCallback(failCallback),
    outputStream(downloadFile.OpenAsync(FileAccessMode::ReadWrite).get()) {}

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

IAsyncAction CodePushDownloadHandler::Download(wstring_view url)
{
    HttpClient client;

    auto headers{ client.DefaultRequestHeaders() };
    headers.Append(L"Accept-Encoding", L"identity");

    HttpRequestMessage reqm{ HttpMethod::Get(), Uri(url) };
    auto resm{ co_await client.SendRequestAsync(reqm, HttpCompletionOption::ResponseHeadersRead) };
    expectedContentLength = resm.Content().Headers().ContentLength().GetInt64();
    auto inputStream{ co_await resm.Content().ReadAsInputStreamAsync() };

    uint8_t header[4];

    for (;;)
    {
        auto outputBuffer{ co_await inputStream.ReadAsync(Buffer{ BufferSize }, BufferSize, InputStreamOptions::None) };

        if (outputBuffer.Length() == 0)
        {
            break;
        }
        co_await outputStream.WriteAsync(outputBuffer);

        if (receivedContentLength < 4)
        {
            for (uint32_t i{ static_cast<uint32_t>(receivedContentLength) }; i < 4 && i < outputBuffer.Length(); i++)
            {
                header[i] = outputBuffer.data()[i];
            }
        }

        receivedContentLength += outputBuffer.Length();

        progressCallback(/*expectedContentLength*/ expectedContentLength, /*receivedContentLength*/ receivedContentLength);
    }

    bool isZip{ header[0] == 'P' && header[1] == 'K' && header[2] == 3 && header[3] == 4 };
    doneCallback(isZip);
}