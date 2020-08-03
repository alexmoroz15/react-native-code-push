#pragma once

#include "NativeModules.h"

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
		std::vector<bool> _restartQueue;

		void LoadBundle();
		void RestartAppInternal(bool onlyIfUpdateIsPending);
		bool IsPendingUpdate(std::wstring& updateHash);

		winrt::Microsoft::ReactNative::ReactNativeHost m_host;
		winrt::Microsoft::ReactNative::ReactInstanceSettings m_instance;

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

		std::wstring GetApplicationSupportDirectory()
		{

		}

		REACT_INIT(Initialize)
		void Initialize(winrt::Microsoft::ReactNative::ReactContext const& reactContext) noexcept;
		/*
		{
			using namespace winrt::Microsoft::ReactNative;

			//auto res = reactContext.Properties().Handle().Get(winrt::Microsoft::ReactNative::ReactPropertyBagHelper::GetName(nullptr, L"MyReactNativeHost"));
			auto res = reactContext.Properties().Handle().Get(ReactPropertyBagHelper::GetName(nullptr, L"MyReactNativeHost"));
			auto host = res.as<ReactNativeHost>();
			m_host = host;
			OutputDebugStringW((host.InstanceSettings().JavaScriptBundleFile() + L"\n").c_str());
			//host.InstanceSettings().JavaScriptBundleFile(L"");
			//host.ReloadInstance();
		}
		*/

		REACT_METHOD(GetNewStatusReport, L"getNewStatusReport")
		void GetNewStatusReport(winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue> promise) noexcept;

		REACT_METHOD(GetUpdateMetadata, L"getUpdateMetadata")
		winrt::fire_and_forget GetUpdateMetadata(CodePushUpdateState updateState, winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue> promise) noexcept;

		REACT_METHOD(RestartApp, L"restartApp")
		void RestartApp(bool onlyIfUpdateIsPending, winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue>&& promise) noexcept;

		REACT_METHOD(NotifyApplicationReady, L"notifyApplicationReady")
		void NotifyApplicationReady(winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue>&& promise) noexcept;
	};
}
