#pragma once

#include <winrt/Windows.Data.Json.h>
#include <filesystem>

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

		IAsyncAction DownloadPackageAsync(
			JsonObject updatePackage,
			wstring expectedBundleFileName,
			wstring publicKey,
			int operationQueue,
			int progressCallback,
			int doneCallback,
			int failCallback);

		static IAsyncOperation<JsonObject> GetCurrentPackageAsync();
		static IAsyncOperation<JsonObject> GetPreviousPackageAsync();
		static IAsyncOperation<path> GetCurrentPackageFolderPathAsync();
		static IAsyncOperation<path> GetCurrentPackageBundlePathAsync();
		static IAsyncOperation<wstring> GetCurrentPackageHashAsync();

		IAsyncOperation<JsonObject> GetPackageAsync(wstring packageHash);

		path GetPackageFolderPath(wstring packageHash);

		IAsyncOperation<bool> InstallPackageAsync(JsonObject updatePackage, bool removePendingUpdate);

		void RollbackPackage();

		// The below methods are only used during tests.
		void ClearUpdates();
		IAsyncAction DownloadAndReplaceCurrentBundleAsync(wstring remoteBundleUrl);
	};
}