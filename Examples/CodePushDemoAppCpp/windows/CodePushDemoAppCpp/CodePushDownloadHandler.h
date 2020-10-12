#pragma once

#include "winrt/Windows.Storage.h"
#include "winrt/Windows.Storage.Streams.h"
#include "winrt/Windows.Data.Json.h"
#include <string>
#include <filesystem>
#include <functional>

/*

@interface CodePushDownloadHandler : NSObject <NSURLConnectionDelegate>

@property (strong) NSOutputStream *outputFileStream;
@property long long expectedContentLength;
@property long long receivedContentLength;
@property dispatch_queue_t operationQueue;
@property (copy) void (^progressCallback)(long long, long long);
@property (copy) void (^doneCallback)(BOOL);
@property (copy) void (^failCallback)(NSError *err);
@property NSString *downloadUrl;

- (id)init:(NSString *)downloadFilePath
operationQueue:(dispatch_queue_t)operationQueue
progressCallback:(void (^)(long long, long long))progressCallback
doneCallback:(void (^)(BOOL))doneCallback
failCallback:(void (^)(NSError *err))failCallback;

- (void)download:(NSString*)url;

@end

*/

namespace CodePush
{
	using namespace winrt;
	using namespace Windows::Storage;
	using namespace Windows::Storage::Streams;

	using namespace std;
	using namespace filesystem;

	struct CodePushDownloadHandler
	{
	private:

	public:
		// OutputStream
		int64_t expectedContentLength;
		int64_t receivedContentLength;
		// Dispatch queue
		function<void(int64_t, int64_t)> progressCallback;
		function<void(bool)> doneCallback;
		function<void(const hresult_error&)> failCallback;
		wstring downloadUrl;

		CodePushDownloadHandler(
			path downloadFilePath,
			function<void(int64_t, int64_t)> progressCallback,
			function<void(bool)> doneCallback,
			function<void(const hresult_error&)> failCallback);

		void Download(wstring_view url);
	};
}