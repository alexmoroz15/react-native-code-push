#pragma once

#include <winrt/Windows.Data.Json.h>

namespace CodePush
{
	struct CodePushPackage
	{
	private:
	public:
		static winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Data::Json::JsonObject> GetCurrentPackageAsync();
		static winrt::Windows::Foundation::IAsyncOperation<winrt::hstring> GetCurrentPackageBundlePathAsync(winrt::hstring bundleFileName);
		static winrt::Windows::Foundation::IAsyncOperation<winrt::hstring> GetCurrentPackageFolderPathAsync();

		static winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Data::Json::JsonObject> GetPreviousPackageAsync();
		static winrt::Windows::Foundation::IAsyncOperation<winrt::hstring> GetPreviousPackageFolderPathAsync();
	};
}