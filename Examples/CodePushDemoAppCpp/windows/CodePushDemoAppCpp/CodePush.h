#pragma once

#include "NativeModules.h"

namespace CodePush
{
	//enum CodePushInstallMode;

	REACT_MODULE(CodePush)
		struct CodePush
	{
	private:
		bool _allowed{ true };
		bool _restartInProgress{ false };
		std::vector<bool> _restartQueue;

		void LoadBundle();
		void RestartAppInternal(bool onlyIfUpdateIsPending);

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
		
		REACT_METHOD(RestartApp, L"restartApp")
		void RestartApp(bool onlyIfUpdateIsPending, winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue>&& promise) noexcept;

		REACT_METHOD(NotifyApplicationReady, L"notifyApplicationReady")
		void NotifyApplicationReady(winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValue> promise) noexcept;
	};
}
