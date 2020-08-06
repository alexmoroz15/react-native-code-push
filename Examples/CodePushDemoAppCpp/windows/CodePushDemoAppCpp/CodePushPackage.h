#pragma once

#include <winrt/Windows.Data.Json.h>

namespace CodePush
{
	struct CodePushPackage
	{
	private:
	public:
		static winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Data::Json::IJsonValue> GetCurrentPackage();
		static winrt::Windows::Foundation::IAsyncOperation<winrt::hstring> GetCurrentPackageBundlePath(winrt::hstring bundleFileName);
		static winrt::Windows::Foundation::IAsyncOperation<winrt::hstring> GetCurrentPackageFolderPath();
	};
}