#include "pch.h"

#include "CodePushPackage.h"

#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Foundation.h>

#include <filesystem>
#include <functional>

using namespace CodePush;

using namespace std;

IAsyncAction CodePushPackage::DownloadPackageAsync(
	JsonObject updatePackage,
	wstring_view expectedBundleFileName,
	wstring_view publicKey,
	function<void(int64_t, int64_t)> progressCallback,
	function<void()> doneCallback,
	function<void(const hresult_error&)> failCallback)
{
	co_return;
}

IAsyncOperation<JsonObject> CodePushPackage::GetPackageAsync(wstring_view packageHash)
{
	co_return nullptr;
}