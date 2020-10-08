#pragma once

#include "NativeModules.h"
#include "winrt/Windows.Data.Json.h"
#include "winrt/Windows.Storage.h"

#include <filesystem>

// Helper functions for reading and sending JsonValues to and from JavaScript
namespace winrt::Microsoft::ReactNative
{
	using namespace winrt::Windows::Data::Json;
	void ReadValue(IJSValueReader const& reader, /*out*/ JsonObject& value) noexcept;
	void ReadValue(IJSValueReader const& reader, /*out*/ IJsonValue& value) noexcept;
}

namespace CodePush
{
	using namespace winrt;
	using namespace Windows::Data::Json;
	using namespace Windows::Foundation;
	using namespace Windows::Storage;
	using namespace Microsoft::ReactNative;

	using namespace std;
	using namespace filesystem;

	REACT_MODULE(CodePush);
	struct CodePush
	{
		enum class CodePushInstallMode;

	private:
		bool m_hasResumeListener;
		bool m_isFirstRunAfterUpdate;
		int m_minimumBackgroundDuration;
		DateTime m_lastResignedDate;
		CodePushInstallMode _installMode;
		// Don't know what to replace this with.
		//NSTimer* _appSuspendTimer;
		

		// Used to coordinate the dispatching of download progress events to JS.
		uint64_t m_latestExpectedContentLength;
		uint64_t m_latestReceivedConentLength;
		bool m_didUpdateProgress;

		bool m_allowed{ true };
		bool m_restartInProgress{ false };
		vector<uint8_t> m_restartQueue;



		// These constants represent emitted events
		const wstring DownloadProgressEvent{ L"CodePushDownloadProgress" };

		// These constants represent valid deployment statuses
		const wstring DeploymentFailed{ L"DeploymentFailed" };
		const wstring DeploymentSucceeded{ L"DeploymentSucceeded" };

		// These keys represent the names we use to store data in NSUserDefaults
		// For Windows, NSUserDefaults is replaced with LocalSettings
		const wstring FailedUpdatesKey{ L"CODE_PUSH_FAILED_UPDATES" };
		const wstring PendingUpdateKey{ L"CODE_PUSH_PENDING_UPDATE" };

		// These keys are already "namespaced" by the PendingUpdateKey, so
		// their values don't need to be obfuscated to prevent collision with app data
		const wstring PendingUpdateHashKey{ L"hash" };
		const wstring PendingUpdateIsLoadingKey{ L"isLoading" };

		// These keys are used to inspect/augment the metadata
		// that is associated with an update's package.
		const wstring AppVersionKey{ L"appVersion" };
		const wstring BinaryBundleDateKey{ L"binaryDate" };
		const wstring PackageHashKey{ L"packageHash" };
		const wstring PackageIsPendingKey{ L"isPending" };


		bool isRunningBinaryVersion{ false };
		bool needToReportRollback{ false };
		bool testConfigurationFlag{ false };

		// These values are used to save the NS bundle, name, extension and subdirectory
		// for the JS bundle in the binary.
		// I won't be doing this.
		/*
		static NSBundle* bundleResourceBundle = nil;
		static NSString* bundleResourceExtension = @"jsbundle";
		static NSString* bundleResourceName = @"main";
		static NSString* bundleResourceSubdirectory = nil;
		*/
		ReactNativeHost m_host;
		ReactContext m_context;

		// These keys represent the names we use to store information about the latest rollback
		const wstring LatestRollbackInfoKey{ L"LATEST_ROLLBACK_INFO" };
		const wstring LatestRollbackPackageHashKey{ L"packageHash" };
		const wstring LatestRollbackTimeKey{ L"time" };
		const wstring LatestRollbackCountKey{ L"count" };

		void LoadBundle();
		void RemovePendingUpdate();
		void RestartAppInternal(bool onlyIfUpdateIsPending);

	public:
		enum class CodePushInstallMode
		{
			IMMEDIATE = 0,
			ON_NEXT_RESTART = 1,
			ON_NEXT_RESUME = 2,
			ON_NEXT_SUSPEND = 3
		};

		enum class CodePushUpdateState
		{
			RUNNING = 0,
			PENDING = 1,
			LATEST = 2
		};

		static path GetBinaryBundlePath();
		static IAsyncOperation<StorageFile> GetBundleFileAsync();
		static path GetBundlePath();

		// Not sure exactly why these methods exist
		/*
		StorageFolder GetLocalStorageFolder();
		path GetBundleAssetsPath();
		*/

		void OverrideAppVersion(wstring appVersion);
		void SetDeploymentKey(wstring deploymentKey);

		bool IsFailedHash(wstring packageHash);

		JsonObject GetRollbackInfo();
		//void SetLatestRollbackInfo(wstring packageHash);
		int GetRollbackCountForPackage(wstring packageHash, JsonObject latestRollbackInfo);

		bool IsPendingUpdate(wstring packageHash);

		bool IsUsingTestConfiguration();
		void SetUsingTestConfiguration();
		//void ClearUpdates();

		REACT_INIT(Initialize);
		void Initialize(winrt::Microsoft::ReactNative::ReactContext const& reactContext) noexcept;

		REACT_CONSTANT_PROVIDER(GetConstants);
		void GetConstants(winrt::Microsoft::ReactNative::ReactConstantProvider& constants) noexcept;

		/*
		 * This is native-side of the RemotePackage.download method
		 */
		REACT_METHOD(DownloadUpdateAsync, L"downloadUpdate");
		fire_and_forget DownloadUpdateAsync(JsonObject updatePackage, bool notifyProgress, ReactPromise<JsonObject> promise) noexcept;

        /*
         * This is the native side of the CodePush.getConfiguration method. It isn't
         * currently exposed via the "react-native-code-push" module, and is used
         * internally only by the CodePush.checkForUpdate method in order to get the
         * app version, as well as the deployment key that was configured in the Info.plist file.
         */
		REACT_METHOD(GetConfiguration, L"getConfiguration");
		void GetConfiguration(ReactPromise<IJsonValue> promise) noexcept;

        /*
         * This method is the native side of the CodePush.getUpdateMetadata method.
         */
		REACT_METHOD(GetUpdateMetadataAsync, L"getUpdateMetadata");
		fire_and_forget GetUpdateMetadataAsync(CodePushUpdateState updateState, ReactPromise<JsonObject> promise) noexcept;

        /*
         * This method is the native side of the LocalPackage.install method.
         */
		REACT_METHOD(InstallUpdateAsync, L"installUpdate");
		fire_and_forget InstallUpdateAsync(JsonObject updatePackage, CodePushInstallMode installMode, int minimumBackgroundDuration, ReactPromise<void> promise) noexcept;

        /*
         * This method isn't publicly exposed via the "react-native-code-push"
         * module, and is only used internally to populate the RemotePackage.failedInstall property.
         */
		REACT_METHOD(IsFirstRun, L"isFirstRun");
		void IsFailedUpdate(wstring packageHash, ReactPromise<bool> promise) noexcept;

		REACT_METHOD(SetLatestRollbackInfo, L"setLatestRollbackInfo");
		void SetLatestRollbackInfo(wstring packageHash) noexcept;

		REACT_METHOD(GetLatestRollbackInfo, L"getLatestRollbackInfo");
		void GetLatestRollbackInfo(ReactPromise<JsonObject> promise) noexcept;

        /*
         * This method isn't publicly exposed via the "react-native-code-push"
         * module, and is only used internally to populate the LocalPackage.isFirstRun property.
         */
		REACT_METHOD(IsFirstRun, L"isFirstRun");
		void IsFirstRun(wstring packageHash, ReactPromise<bool> promise) noexcept;

        /*
         * This method is the native side of the CodePush.notifyApplicationReady() method.
         */
		REACT_METHOD(NotifyApplicationReady, L"notifyApplicationReady");
		void NotifyApplicationReady(ReactPromise<IJsonValue> promise) noexcept;

		REACT_METHOD(Allow, L"allow");
		void Allow(ReactPromise<JSValue> promise) noexcept;

		REACT_METHOD(ClearPendingRestart, L"clearPendingRestart");
		void ClearPendingRestart() noexcept;

		REACT_METHOD(Disallow, L"disallow");
		void Disallow(ReactPromise<JSValue> promise) noexcept;

        /*
         * This method is the native side of the CodePush.restartApp() method.
         */
		REACT_METHOD(RestartApp, L"restartApp");
		void RestartApp(bool onlyIfUpdateIsPending, ReactPromise<JSValue> promise) noexcept;

        /*
         * This method clears CodePush's downloaded updates.
         * It is needed to switch to a different deployment if the current deployment is more recent.
         * Note: we don’t recommend to use this method in scenarios other than that (CodePush will call this method
         * automatically when needed in other cases) as it could lead to unpredictable behavior.
         */
		REACT_METHOD(ClearUpdates, L"clearUpdates");
		void ClearUpdates() noexcept;

    // #pragma mark - JavaScript-exported module methods (Private)

        /*
         * This method is the native side of the CodePush.downloadAndReplaceCurrentBundle()
         * method, which replaces the current bundle with the one downloaded from
         * removeBundleUrl. It is only to be used during tests and no-ops if the test
         * configuration flag is not set.
         */
		REACT_METHOD(DownloadAndReplaceCurrentBundle, L"downloadAndReplaceCurrentBundle");
		void DownloadAndReplaceCurrentBundle(wstring remoteBundleUrl) noexcept;

        /*
         * This method is checks if a new status update exists (new version was installed,
         * or an update failed) and return its details (version label, status).
         */
		REACT_METHOD(GetNewStatusReportAsync, L"getNewStatusReport");
		fire_and_forget GetNewStatusReportAsync(ReactPromise<IJsonValue> promise) noexcept;

		REACT_METHOD(RecordStatusReported, L"recordStatusReported");
		void RecordStatusReported(JsonObject statusReport) noexcept;

		REACT_METHOD(SaveStatusReportForRetry, L"saveStatusReportForRetry");
		void SaveStatusReportForRetry(JsonObject statusReport) noexcept;

	};
}


