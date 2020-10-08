#pragma once

#include <winrt/Windows.Data.Json.h>
#include <filesystem>
#include <functional>

/*
@interface CodePushPackage : NSObject

+ (void)downloadPackage:(NSDictionary *)updatePackage
 expectedBundleFileName:(NSString *)expectedBundleFileName
			  publicKey:(NSString *)publicKey
		 operationQueue:(dispatch_queue_t)operationQueue
	   progressCallback:(void (^)(long long, long long))progressCallback
		   doneCallback:(void (^)())doneCallback
		   failCallback:(void (^)(NSError *err))failCallback;

+ (NSDictionary *)getCurrentPackage:(NSError **)error;
+ (NSDictionary *)getPreviousPackage:(NSError **)error;
+ (NSString *)getCurrentPackageFolderPath:(NSError **)error;
+ (NSString *)getCurrentPackageBundlePath:(NSError **)error;
+ (NSString *)getCurrentPackageHash:(NSError **)error;

+ (NSDictionary *)getPackage:(NSString *)packageHash
					   error:(NSError **)error;

+ (NSString *)getPackageFolderPath:(NSString *)packageHash;

+ (BOOL)installPackage:(NSDictionary *)updatePackage
   removePendingUpdate:(BOOL)removePendingUpdate
				 error:(NSError **)error;

+ (void)rollbackPackage;

// The below methods are only used during tests.
+ (void)clearUpdates;
+ (void)downloadAndReplaceCurrentBundle:(NSString *)remoteBundleUrl;

@end
*/

namespace CodePush
{
	using namespace winrt;
	using namespace Windows::Data::Json;
	using namespace Windows::Foundation;
	using namespace Microsoft::ReactNative;

	using namespace std;
	using namespace filesystem;

	struct CodePushPackage
	{
	private:

		const wstring DiffManifestFileName{ L"hotcodepush.json" };
		const wstring DownloadFileName{ L"download.zip" };
		const wstring RelativeBundlePathKey{ L"bundlePath" };
		const wstring StatusFile{ L"codepush.json" };
		const wstring UpdateBundleFileName{ L"app.jsbundle" };
		const wstring UpdateMetadataFileName{ L"app.json" };
		const wstring UnzippedFolderName{ L"unzipped" };

	public:

		static IAsyncAction DownloadPackageAsync(
			JsonObject updatePackage,
			wstring_view expectedBundleFileName,
			wstring_view publicKey,
			function<void(int64_t, int64_t)> progressCallback,
			function<void()> doneCallback,
			function<void(const hresult_error&)> failCallback);

		static IAsyncOperation<JsonObject> GetCurrentPackageAsync();
		static IAsyncOperation<JsonObject> GetPreviousPackageAsync();
		static IAsyncOperation<path> GetCurrentPackageFolderPathAsync();
		static IAsyncOperation<path> GetCurrentPackageBundlePathAsync();
		static IAsyncOperation<wstring> GetCurrentPackageHashAsync();

		static IAsyncOperation<JsonObject> GetPackageAsync(wstring_view packageHash);

		static path GetPackageFolderPath(wstring_view packageHash);

		static IAsyncOperation<bool> InstallPackageAsync(JsonObject updatePackage, bool removePendingUpdate);

		static void RollbackPackage();

		// The below methods are only used during tests.
		static void ClearUpdates();
		static IAsyncAction DownloadAndReplaceCurrentBundleAsync(wstring remoteBundleUrl);
	};
}