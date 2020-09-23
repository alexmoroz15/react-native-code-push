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

	REACT_MODULE(CodePush);
	struct CodePush
	{
	private:
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
		winrt::fire_and_forget GetUpdateMetadata(CodePushUpdateState updateState, winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue> promise) noexcept;

		REACT_METHOD(IsFailedUpdate, L"isFailedUpdate");
		void IsFailedUpdate(std::wstring packageHash, winrt::Microsoft::ReactNative::ReactPromise<bool> promise) noexcept;

		REACT_METHOD(RestartApp, L"restartApp");
		void RestartApp(bool onlyIfUpdateIsPending, winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue>&& promise) noexcept;

		REACT_METHOD(NotifyApplicationReady, L"notifyApplicationReady");
		void NotifyApplicationReady(winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue>&& promise) noexcept;

		REACT_METHOD(DownloadUpdate, L"downloadUpdate");
		winrt::fire_and_forget DownloadUpdate(winrt::Microsoft::ReactNative::JSValueObject updatePackage, bool notifyProgress, winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue> promise) noexcept;

		REACT_METHOD(InstallUpdate, L"installUpdate");
		winrt::fire_and_forget InstallUpdate(winrt::Microsoft::ReactNative::JSValueObject updatePackage, int installMode, int minimumBackgroundDuration, winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue> promise) noexcept;

		winrt::Windows::Foundation::IAsyncAction CodePush::CodePush::InstallPackage(winrt::Microsoft::ReactNative::JSValueObject updatePackage);

		REACT_METHOD(ClearPendingRestart, L"clearPendingRestart");
		void ClearPendingRestart(winrt::Microsoft::ReactNative::ReactPromise<void> promise) noexcept;
	};
}
