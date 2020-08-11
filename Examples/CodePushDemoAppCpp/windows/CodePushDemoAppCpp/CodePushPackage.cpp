#include "pch.h"

#include "CodePushPackage.h"

#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Foundation.h>

#include <filesystem>

using namespace winrt;
using namespace Windows::Data::Json;
using namespace Windows::Foundation;
using namespace Windows::Storage;

using namespace std;
using namespace filesystem;

const winrt::hstring CodePushFolderPrefix{ L"CodePush" };
const winrt::hstring StatusFile{ L"codepush.json" };
const winrt::hstring UpdateMetadataFileName{ L"app.json" };

path GetLocalDataPath()
{
	return winrt::Windows::Storage::ApplicationData::Current().LocalFolder().Path().c_str();
}

path GetCodePushPath()
{
	return GetLocalDataPath() / CodePushFolderPrefix.c_str();
}

path GetStatusFilePath()
{
	return GetCodePushPath() / StatusFile.c_str();
}

IAsyncOperation<IJsonValue> GetCurrentPackageInfo()
{
	co_return JsonValue::CreateNullValue();
	/*
	try
	{
		auto statusFilePath = GetStatusFilePath();
		auto file = co_await StorageFile::GetFileFromPathAsync(statusFilePath.c_str());
		auto content = co_await FileIO::ReadTextAsync(file);
		auto json = JsonObject::Parse(content);
		co_return json;
	}
	catch (...)
	{
		OutputDebugStringW(L"Error has occurred.\n");
		co_return JsonValue::CreateNullValue();
	}
	*/
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

path GetPackageFolderPath(winrt::hstring& packageHash)
{
	return GetCodePushPath() / packageHash.c_str();
}

IAsyncOperation<IJsonValue> GetPackage(winrt::hstring& packageHash)
{
	try
	{
		auto updateDirectoryPath = GetPackageFolderPath(packageHash);
		auto updateMetadataFilePath = updateDirectoryPath / UpdateMetadataFileName.c_str();

		auto file = co_await StorageFile::GetFileFromPathAsync(updateMetadataFilePath.c_str());
		auto content = co_await FileIO::ReadTextAsync(file);
		auto json = JsonObject::Parse(content);
		co_return json;
	}
	catch (...)
	{
		co_return JsonValue::CreateNullValue();
	}
}

/*
+ (NSDictionary *)getPackage:(NSString *)packageHash
					   error:(NSError **)error
{
	NSString *updateDirectoryPath = [self getPackageFolderPath:packageHash];
	NSString *updateMetadataFilePath = [updateDirectoryPath stringByAppendingPathComponent:UpdateMetadataFileName];

	if (![[NSFileManager defaultManager] fileExistsAtPath:updateMetadataFilePath]) {
		return nil;
	}

	NSString *updateMetadataString = [NSString stringWithContentsOfFile:updateMetadataFilePath
															   encoding:NSUTF8StringEncoding
																  error:error];
	if (!updateMetadataString) {
		return nil;
	}

	NSData *updateMetadata = [updateMetadataString dataUsingEncoding:NSUTF8StringEncoding];
	return [NSJSONSerialization JSONObjectWithData:updateMetadata
										   options:kNilOptions
											 error:error];
}
*/

IAsyncOperation<hstring> CodePush::CodePushPackage::GetCurrentPackageFolderPath()
{
	co_return L"";
}

IAsyncOperation<IJsonValue> CodePush::CodePushPackage::GetCurrentPackage()
{
	auto packageHash = co_await GetCurrentPackageHash();
	if (packageHash.empty())
	{
		co_return JsonValue::CreateNullValue();
	}

	co_return co_await GetPackage(packageHash);
}


IAsyncOperation<hstring> CodePush::CodePushPackage::GetCurrentPackageBundlePath(hstring bundleFileName)
{
	auto packageFolder = co_await GetCurrentPackageFolderPath();
	if (packageFolder.empty())
	{
		co_return L"";
	}

	auto currentPackage = co_await GetCurrentPackage();
	co_return L"";
}
/*
public String getCurrentPackageBundlePath(String bundleFileName) {
		String packageFolder = getCurrentPackageFolderPath();
		if (packageFolder == null) {
			return null;
		}

		JSONObject currentPackage = getCurrentPackage();
		if (currentPackage == null) {
			return null;
		}

		String relativeBundlePath = currentPackage.optString(CodePushConstants.RELATIVE_BUNDLE_PATH_KEY, null);
		if (relativeBundlePath == null) {
			return CodePushUtils.appendPathComponent(packageFolder, bundleFileName);
		} else {
			return CodePushUtils.appendPathComponent(packageFolder, relativeBundlePath);
		}
	}
*/