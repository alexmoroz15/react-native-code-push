#include "pch.h"

#include "CodePushPackage.h"

#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Foundation.h>

#include <filesystem>

using namespace winrt::Windows::Data::Json;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Storage;

using namespace std;
using namespace filesystem;

const std::wstring CodePushFolderPrefix{ L"CodePush" };
const winrt::hstring StatusFile{ L"codepush.json" };

path GetLocalDataPath()
{
	return winrt::Windows::Storage::ApplicationData::Current().LocalFolder().Path().c_str();
	//return Uri(winrt::Windows::Storage::ApplicationData::Current().LocalFolder().Path());
}

path GetCodePushPath()
{
	return GetLocalDataPath() / CodePushFolderPrefix.c_str();
	//return Uri(GetLocalDataPath().AbsoluteUri(), CodePushFolderPrefix);
}

path GetStatusFilePath()
{
	return GetCodePushPath() / StatusFile.c_str();
	//return Uri(GetCodePushPath().AbsoluteUri(), StatusFile);
}

IAsyncOperation<IJsonValue> GetCurrentPackageInfo()
{
	try
	{
		auto statusFilePath = GetStatusFilePath();
		//auto file = co_await StorageFile::GetFileFromPathAsync(statusFilePath.AbsoluteUri());
		auto file = co_await StorageFile::GetFileFromPathAsync(statusFilePath.c_str());
		auto content = co_await FileIO::ReadTextAsync(file);
		auto json = JsonObject::Parse(content);
		co_return json;
	}
	catch (...)
	{
		//co_return JsonValue::CreateNullValue;
		OutputDebugStringW(L"Error has occurred.\n");
		co_return JsonValue::CreateNullValue();
	}
	//return JsonValue::CreateNullValue();
}

IAsyncOperation<winrt::hstring> GetCurrentPackageHash()
{
	auto info = co_await GetCurrentPackageInfo();
	if (info.ValueType() == JsonValueType::Null)
	{
		co_return L"";
	}

	co_return info.GetObject().GetNamedString(L"currentPackage");
}

JsonValue GetPackage(winrt::hstring)
{
	return JsonValue::CreateNullValue();
}

IAsyncOperation<JsonValue> CodePush::CodePushPackage::GetCurrentPackage()
{
	auto packageHash = co_await GetCurrentPackageHash();
	if (packageHash.empty())
	{
		return JsonValue::CreateNullValue();
	}

	return GetPackage(packageHash);
}