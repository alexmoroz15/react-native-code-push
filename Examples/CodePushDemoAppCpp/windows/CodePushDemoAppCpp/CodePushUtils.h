#pragma once

#include "NativeModules.h"

namespace CodePush
{
	struct CodePushUtils
	{
		static void Log(winrt::hstring message);
		static void Log(std::exception error);
		static void LogBundleUrl(winrt::hstring path);
	};
}