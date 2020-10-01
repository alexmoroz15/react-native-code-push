#pragma once

#include "NativeModules.h"
#include "winrt/Windows.Data.Json.h"
#include <filesystem>

namespace CodePush
{
	using JsonObject = winrt::Windows::Data::Json::JsonObject;
	using path = std::filesystem::path;
	using wstring = std::wstring;

	using ReactNativeHost = winrt::Microsoft::ReactNative::ReactNativeHost;
	using ReactContext = winrt::Microsoft::ReactNative::ReactContext;

	using DateTime = winrt::Windows::Foundation::DateTime;

	REACT_MODULE(CodePush);
	struct CodePush
	{
		enum class CodePushInstallMode;

	private:
		/*
		const std::wstring PendingUpdatekey{ L"CODE_PUSH_PENDING_UPDATE" };

		const std::wstring PendingUpdateHashKey{ L"hash" };
		const std::wstring PendingUpdateIsLoadingKey{ L"isLoading" };
		const winrt::hstring DefaultJSBundleName{ L"index.windows" };
		const winrt::hstring AssetsBundlePrefix{ L"ms-appx:///" };
		
		bool _allowed{ true };
		bool _restartInProgress{ false };
		static inline bool _testConfigurationFlag{ false };
		std::vector<uint8_t> _restartQueue;

		winrt::Microsoft::ReactNative::ReactNativeHost m_host;
		winrt::Microsoft::ReactNative::ReactContext m_context;

		winrt::hstring m_assetsBundleFileName;
		winrt::hstring m_ClientUniqueId;
		std::string m_BinaryContentsHash{ "" };

		static bool IsUsingTestConfiguration();
		static void SetUsingTestConfiguration(bool shouldUseTestConfiguration);

		winrt::fire_and_forget LoadBundle();
		void RestartAppInternal(bool onlyIfUpdateIsPending);
		bool IsPendingUpdate(winrt::hstring&& updateHash);

		winrt::Windows::Foundation::IAsyncOperation<winrt::hstring> GetJSBundleFile();
		winrt::Windows::Foundation::IAsyncOperation<winrt::hstring> GetJSBundleFile(winrt::hstring assetsBundleFileName);
		void SetJSBundleFile(winrt::hstring latestJSBundleFile);
		*/

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
		std::vector<uint8_t> m_restartQueue;



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
		static winrt::Windows::Foundation::IAsyncOperation<path> GetBundlePathAsync();
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
		void SetLatestRollbackInfo(wstring packageHash);
		int GetRollbackCountForPackage(wstring packageHash, JsonObject latestRollbackInfo);

		bool IsPendingUpdate(wstring packageHash);

		bool IsUsingTestConfiguration();
		void SetUsingTestConfiguration();
		void ClearUpdates();

		REACT_INIT(Initialize);
		void Initialize(winrt::Microsoft::ReactNative::ReactContext const& reactContext) noexcept;

		REACT_CONSTANT_PROVIDER(GetConstants);
		void GetConstants(winrt::Microsoft::ReactNative::ReactConstantProvider& constants) noexcept
		{
			constants.Add(L"codePushInstallModeImmediate", CodePushInstallMode::IMMEDIATE);
			constants.Add(L"codePushInstallModeOnNextRestart", CodePushInstallMode::ON_NEXT_RESTART);
			constants.Add(L"codePushInstallModeOnNextResume", CodePushInstallMode::ON_NEXT_RESUME);
			constants.Add(L"codePushInstallModeOnNextSuspend", CodePushInstallMode::ON_NEXT_SUSPEND);

			constants.Add(L"codePushUpdateStateRunning", CodePushUpdateState::RUNNING);
			constants.Add(L"codePushUpdateStatePending", CodePushUpdateState::PENDING);
			constants.Add(L"codePushUpdateStateLatest", CodePushUpdateState::LATEST);
		}

		/*
		// Synchronous functions are not allowed on the UI thread.
		// Incidentally, the application is initialized and the bundle loaded by the UI thread.
		static winrt::hstring GetJSBundleFileSync();

		REACT_CONSTANT_PROVIDER(GetConstants);
		void GetConstants(winrt::Microsoft::ReactNative::ReactConstantProvider& constants) noexcept
		{
			constants.Add(L"codePushInstallModeImmediate", CodePushInstallMode::IMMEDIATE);
			constants.Add(L"codePushInstallModeOnNextRestart", CodePushInstallMode::ON_NEXT_RESTART);
			constants.Add(L"codePushInstallModeOnNextResume", CodePushInstallMode::ON_NEXT_RESUME);
			constants.Add(L"codePushInstallModeOnNextSuspend", CodePushInstallMode::ON_NEXT_SUSPEND);

			constants.Add(L"codePushUpdateStateRunning", CodePushUpdateState::RUNNING);
			constants.Add(L"codePushUpdateStatePending", CodePushUpdateState::PENDING);
			constants.Add(L"codePushUpdateStateLatest", CodePushUpdateState::LATEST);
		}

		REACT_INIT(Initialize);
		void Initialize(winrt::Microsoft::ReactNative::ReactContext const& reactContext) noexcept;

		REACT_METHOD(Allow, L"allow");
		void Allow(winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue>&& promise) noexcept;

		REACT_METHOD(Disallow, L"disallow");
		void Disallow(winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue>&& promise) noexcept;

		REACT_METHOD(GetConfiguration, L"getConfiguration");
		void GetConfiguration(winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue>&& promise) noexcept;

		REACT_METHOD(GetNewStatusReport, L"getNewStatusReport");
		void GetNewStatusReport(winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue>&& promise) noexcept;

		REACT_METHOD(GetUpdateMetadata, L"getUpdateMetadata");
		winrt::fire_and_forget GetUpdateMetadata(CodePushUpdateState updateState, winrt::Microsoft::ReactNative::ReactPromise<winrt::Windows::Data::Json::IJsonValue> promise) noexcept;

		REACT_METHOD(IsFailedUpdate, L"isFailedUpdate");
		void IsFailedUpdate(std::wstring packageHash, winrt::Microsoft::ReactNative::ReactPromise<bool> promise) noexcept;

		REACT_METHOD(RestartApp, L"restartApp");
		void RestartApp(bool onlyIfUpdateIsPending, winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue>&& promise) noexcept;

		REACT_METHOD(IsFirstRun, L"isFirstRun");
		winrt::fire_and_forget IsFirstRun(std::wstring packageHash, winrt::Microsoft::ReactNative::ReactPromise<bool> promise) noexcept;

		REACT_METHOD(NotifyApplicationReady, L"notifyApplicationReady");
		void NotifyApplicationReady(winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue>&& promise) noexcept;

		REACT_METHOD(DownloadUpdate, L"downloadUpdate");
		winrt::fire_and_forget DownloadUpdate(winrt::Microsoft::ReactNative::JSValueObject updatePackage, bool notifyProgress, winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue> promise) noexcept;

		REACT_METHOD(InstallUpdate, L"installUpdate");
		winrt::fire_and_forget InstallUpdate(winrt::Microsoft::ReactNative::JSValueObject updatePackage, int installMode, int minimumBackgroundDuration, winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue> promise) noexcept;

		winrt::Windows::Foundation::IAsyncAction CodePush::CodePush::InstallPackage(winrt::Microsoft::ReactNative::JSValueObject updatePackage);

		REACT_METHOD(ClearPendingRestart, L"clearPendingRestart");
		void ClearPendingRestart(winrt::Microsoft::ReactNative::ReactPromise<void> promise) noexcept;
		*/
	};
}
