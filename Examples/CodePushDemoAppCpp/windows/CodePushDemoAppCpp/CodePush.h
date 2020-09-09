#pragma once

#include "NativeModules.h"

//#include <filesystem>

namespace CodePush
{
	enum class CodePushUpdateState
	{
		Running,
		Pending,
		Latest
	};

	REACT_MODULE(CodePush)
		struct CodePush
	{
	private:
		const std::wstring PendingUpdatekey{ L"CODE_PUSH_PENDING_UPDATE" };

		const std::wstring PendingUpdateHashKey{ L"hash" };
		const std::wstring PendingUpdateIsLoadingKey{ L"isLoading" };
		const winrt::hstring DefaultJSBundleName{ L"index.windows" };
		const winrt::hstring AssetsBundlePrefix{ L"ms-appx:///" };
		/*
		// These constants represent emitted events
		static NSString* const DownloadProgressEvent = @"CodePushDownloadProgress";

		// These constants represent valid deployment statuses
		static NSString* const DeploymentFailed = @"DeploymentFailed";
		static NSString* const DeploymentSucceeded = @"DeploymentSucceeded";

		// These keys represent the names we use to store data in NSUserDefaults
		static NSString* const FailedUpdatesKey = @"CODE_PUSH_FAILED_UPDATES";
		static NSString* const PendingUpdateKey = @"CODE_PUSH_PENDING_UPDATE";

		// These keys are already "namespaced" by the PendingUpdateKey, so
		// their values don't need to be obfuscated to prevent collision with app data
		static NSString* const PendingUpdateHashKey = @"hash";
		static NSString* const PendingUpdateIsLoadingKey = @"isLoading";

		// These keys are used to inspect/augment the metadata
		// that is associated with an update's package.
		static NSString* const AppVersionKey = @"appVersion";
		static NSString* const BinaryBundleDateKey = @"binaryDate";
		static NSString* const PackageHashKey = @"packageHash";
		static NSString* const PackageIsPendingKey = @"isPending";
		*/

		bool _allowed{ true };
		bool _restartInProgress{ false };
		static inline bool _testConfigurationFlag{ false };
		std::vector<uint8_t> _restartQueue;

		winrt::Microsoft::ReactNative::ReactNativeHost m_host;
		winrt::Microsoft::ReactNative::ReactContext m_context;

		//winrt::Microsoft::ReactNative::ReactInstanceSettings m_instance;
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


	public:
		/*
		REACT_CONSTANT(CodePushInstallModeImmediate, L"codePushInstallModeImmediate")
			const int CodePushInstallModeImmediate{ CodePushInstallMode::Immediate };
		REACT_CONSTANT(CodePushInstallModeOnNextRestart, L"codePushInstallModeOnNextRestart")
			const int CodePushInstallModeOnNextRestart{ CodePushInstallMode::OnNextRestart };
		REACT_CONSTANT(CodePushInstallModeOnNextResume, L"codePushInstallModeOnNextResume")
			const int CodePushInstallModeOnNextResume{ CodePushInstallMode::OnNextResume };
		REACT_CONSTANT(CodePushInstallModeOnNextSuspend, L"codePushInstallModeOnNextSuspend")
			const int CodePushInstallModeOnNextSuspend{ CodePushInstallMode::OnNextSuspend };
		*/

		//std::wstring GetApplicationSupportDirectory();

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
		winrt::fire_and_forget GetUpdateMetadata(CodePushUpdateState updateState, winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue> promise) noexcept;

		REACT_METHOD(IsFailedUpdate, L"isFailedUpdate");
		void IsFailedUpdate(std::wstring packageHash, winrt::Microsoft::ReactNative::ReactPromise<bool> promise) noexcept;

		REACT_METHOD(RestartApp, L"restartApp");
		void RestartApp(bool onlyIfUpdateIsPending, winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue>&& promise) noexcept;

		REACT_METHOD(NotifyApplicationReady, L"notifyApplicationReady");
		void NotifyApplicationReady(winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue>&& promise) noexcept;

		REACT_METHOD(DownloadUpdate, L"downloadUpdate");
		winrt::fire_and_forget DownloadUpdate(winrt::Microsoft::ReactNative::JSValueObject updatePackage, bool notifyProgress, winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue> promise) noexcept;
	};
}
