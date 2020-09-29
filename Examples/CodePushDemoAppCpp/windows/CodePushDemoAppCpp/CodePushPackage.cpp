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
const winrt::hstring AssetsFolderName{ L"assets" };

path GetLocalDataPath()
{
	return winrt::Windows::Storage::ApplicationData::Current().LocalFolder().Path().c_str();
}
/*
path GetCodePushPath()
{
	return GetLocalDataPath() / CodePushFolderPrefix.c_str();
}
*/
path GetStatusFilePath()
{
	//return GetCodePushPath() / StatusFile.c_str();
	return GetLocalDataPath() / StatusFile.c_str();
}

// Returns the contents of app.json
IAsyncOperation<JsonObject> GetCurrentPackageInfo()
{
	auto localStorage{ ApplicationData::Current().LocalFolder() };
	auto currentPackageInfoFile{ (co_await localStorage.TryGetItemAsync(L"codepush.json")).try_as<StorageFile>() };
	if (currentPackageInfoFile == nullptr)
	{
		co_return nullptr;
	}
	auto currentPackageInfoContents{ co_await FileIO::ReadTextAsync(currentPackageInfoFile) };
	
	JsonObject currentPackageInfoObject;
	auto success{ JsonObject::TryParse(currentPackageInfoContents, currentPackageInfoObject) };
	if (!success)
	{
		co_return nullptr;
	}

	co_return currentPackageInfoObject;

	//co_return JsonValue::CreateNullValue();
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
	if (info == nullptr)
	{
		co_return L"";
	}

	co_return info.GetObject().GetNamedString(L"currentPackage");
}

IAsyncOperation<winrt::hstring> GetPreviousPackageHashAsync()
{
	auto info = co_await GetCurrentPackageInfo();
	if (info == nullptr)
	{
		co_return L"";
	}

	co_return info.GetObject().GetNamedString(L"previousPackage", L"");
}

path GetPackageFolderPath(winrt::hstring& packageHash)
{
	//return GetCodePushPath() / packageHash.c_str();
	return GetLocalDataPath() / packageHash.c_str();
}

IAsyncOperation<JsonObject> GetPackage(winrt::hstring& packageHash)
{
	try
	{
		auto updateDirectoryPath = GetPackageFolderPath(packageHash);
		auto updateMetadataFilePath = updateDirectoryPath / CodePushFolderPrefix.c_str() / AssetsFolderName.c_str() / UpdateMetadataFileName.c_str();

		auto file = co_await StorageFile::GetFileFromPathAsync(updateMetadataFilePath.c_str());
		auto content = co_await FileIO::ReadTextAsync(file);
		auto json = JsonObject::Parse(content);
		co_return json;
	}
	catch (...)
	{
		co_return nullptr;
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

IAsyncOperation<hstring> CodePush::CodePushPackage::GetCurrentPackageFolderPathAsync()
{
	auto packageHash{ co_await GetCurrentPackageHash() };
	auto currentPackageFolderPath{ GetPackageFolderPath(packageHash) };
	hstring currentPackageFolderString{ currentPackageFolderPath.c_str() };
	co_return currentPackageFolderString;
}

IAsyncOperation<JsonObject> CodePush::CodePushPackage::GetCurrentPackageAsync()
{
	auto packageHash = co_await GetCurrentPackageHash();
	if (packageHash.empty())
	{
		co_return nullptr;
	}

	co_return co_await GetPackage(packageHash);
}


IAsyncOperation<hstring> CodePush::CodePushPackage::GetCurrentPackageBundlePathAsync(hstring bundleFileName)
{
	auto packageFolder = co_await GetCurrentPackageFolderPathAsync();
	if (packageFolder.empty())
	{
		co_return L"";
	}

	auto currentPackage = co_await GetCurrentPackageAsync();
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

IAsyncOperation<JsonObject> CodePush::CodePushPackage::GetPreviousPackageAsync()
{
	auto packageHash{ co_await GetPreviousPackageHashAsync() };
	if (packageHash.empty())
	{
		co_return nullptr;
	}
	co_return co_await GetPackage(packageHash);
}

IAsyncOperation<hstring> CodePush::CodePushPackage::GetPreviousPackageFolderPathAsync()
{
	auto packageHash{ co_await GetPreviousPackageHashAsync() };
	auto previousPackageFolderPath{ GetPackageFolderPath(packageHash) };
	hstring previousPackageFolderString{ previousPackageFolderPath.c_str() };
	co_return previousPackageFolderString;
}