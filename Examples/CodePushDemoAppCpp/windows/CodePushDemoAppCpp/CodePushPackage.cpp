#include "pch.h"

#include "CodePushDownloadHandler.h"
#include "CodePushNativeModule.h"
#include "CodePushPackage.h"
#include "CodePushUtils.h"
//#include "JSValueAdditions.h"
#include "FileUtils.h"

//#include "NativeModules.h"
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Foundation.h>

#include <filesystem>
#include <functional>
#include <stack>

using namespace CodePush;

using namespace winrt;
using namespace Windows::Data::Json;
using namespace Windows::Storage;
using namespace Windows::Foundation;

using namespace std;

//const wstring DiffManifestFileName{ L"hotcodepush.json" };
//const wstring DownloadFileName{ L"download.zip" };
//const wstring RelativeBundlePathKey{ L"bundlePath" };
//const wstring StatusFile{ L"codepush.json" };
//const wstring UpdateBundleFileName{ L"app.jsbundle" };
//const wstring UnzippedFolderName{ L"unzipped" };

StorageFolder GetCodePushFolder();
path GetCodePushPath();
IAsyncOperation<JsonObject> GetCurrentPackageInfoAsync();
path GetDownloadFilePath();
IAsyncOperation<hstring> GetPreviousPackageHashAsync();
StorageFile GetStatusFile();
path GetStatusFilePath();
StorageFolder GetUnzippedFolder();
path GetUnzippedFolderPath();

IAsyncAction CodePushPackage::DownloadPackageAsync(
	JsonObject updatePackage,
	wstring_view expectedBundleFileName,
	wstring_view publicKey,
	function<void(int64_t, int64_t)> progressCallback,
	function<void()> doneCallback,
	function<void(const hresult_error&)> failCallback)
{
    // Major rewrite needed! All this runs on UI thread! Do not call async funcs synchronously!
    // We can keep the progressCallback and failCallback, but the doneCallback should be appended to the end of this method.

    auto newUpdateHash{ updatePackage.GetNamedString(L"packageHash") };
    auto codePushFolder{ GetCodePushFolder() };
    //auto newUpdateFolder{ co_await codePushFolder.CreateFolderAsync(newUpdateHash, CreationCollisionOption::ReplaceExisting) };
    //auto newUpdateMetadataFile{ co_await newUpdateFolder.CreateFileAsync(UpdateMetadataFileName, CreationCollisionOption::ReplaceExisting) };

    //auto downloadFilePath{ GetDownloadFilePath() };
    //auto bundleFilePath{ path(newUpdateFolder.Path().c_str()) / UpdateBundleFileName };
    auto downloadFile{ codePushFolder.CreateFileAsync(DownloadFileName, CreationCollisionOption::ReplaceExisting).get() };

    CodePushDownloadHandler downloadHandler{
        downloadFile,
        progressCallback, 
        [=](bool isZip) 
        {
            StorageFolder newUpdateFolder{ nullptr };
            StorageFile newUpdateMetadataFile{ nullptr };
            auto unzippedFolderPath{ GetUnzippedFolderPath() };
            auto mutableUpdatePackage{ updatePackage };
            if (isZip)
            {
                auto unzippedFolder{ codePushFolder.CreateFolderAsync(UnzippedFolderName, CreationCollisionOption::ReplaceExisting).get() };
                auto downloadFile{ codePushFolder.GetFileAsync(DownloadFileName).get() };
                FileUtils::UnzipAsync(downloadFile, unzippedFolder).get();
                downloadFile.DeleteAsync();
                
                auto isDiffUpdate{ false };
                auto diffManifestFile{ unzippedFolder.GetItemAsync(DiffManifestFileName).get().try_as<StorageFile>() };
                if (diffManifestFile != nullptr)
                {
                    isDiffUpdate = true;
                }

                if (isDiffUpdate)
                {
                    /*
                    // Copy the current package to the new package.
                    NSString *currentPackageFolderPath = [self getCurrentPackageFolderPath:&error];
                    if (error) {
                        failCallback(error);
                        return;
                    }
                                                            
                    if (currentPackageFolderPath == nil) {
                        // Currently running the binary version, copy files from the bundled resources
                        NSString *newUpdateCodePushPath = [newUpdateFolderPath stringByAppendingPathComponent:[CodePushUpdateUtils manifestFolderPrefix]];
                        [[NSFileManager defaultManager] createDirectoryAtPath:newUpdateCodePushPath
                                                    withIntermediateDirectories:YES
                                                                    attributes:nil
                                                                        error:&error];
                        if (error) {
                            failCallback(error);
                            return;
                        }
                                                                
                        [[NSFileManager defaultManager] copyItemAtPath:[CodePush bundleAssetsPath]
                                                                toPath:[newUpdateCodePushPath stringByAppendingPathComponent:[CodePushUpdateUtils assetsFolderName]]
                                                                    error:&error];
                        if (error) {
                            failCallback(error);
                            return;
                        }
                                                                
                        [[NSFileManager defaultManager] copyItemAtPath:[[CodePush binaryBundleURL] path]
                                                                toPath:[newUpdateCodePushPath stringByAppendingPathComponent:[[CodePush binaryBundleURL] lastPathComponent]]
                                                                    error:&error];
                        if (error) {
                            failCallback(error);
                            return;
                        }
                    } else {
                        [[NSFileManager defaultManager] copyItemAtPath:currentPackageFolderPath
                                                                toPath:newUpdateFolderPath
                                                                    error:&error];
                        if (error) {
                            failCallback(error);
                            return;
                        }
                    }
                                                            
                    // Delete files mentioned in the manifest.
                    NSString *manifestContent = [NSString stringWithContentsOfFile:diffManifestFilePath
                                                                            encoding:NSUTF8StringEncoding
                                                                                error:&error];
                    if (error) {
                        failCallback(error);
                        return;
                    }
                                                            
                    NSData *data = [manifestContent dataUsingEncoding:NSUTF8StringEncoding];
                    NSDictionary *manifestJSON = [NSJSONSerialization JSONObjectWithData:data
                                                                                    options:kNilOptions
                                                                                    error:&error];
                    NSArray *deletedFiles = manifestJSON[@"deletedFiles"];
                    for (NSString *deletedFileName in deletedFiles) {
                        NSString *absoluteDeletedFilePath = [newUpdateFolderPath stringByAppendingPathComponent:deletedFileName];
                        if ([[NSFileManager defaultManager] fileExistsAtPath:absoluteDeletedFilePath]) {
                            [[NSFileManager defaultManager] removeItemAtPath:absoluteDeletedFilePath
                                                                        error:&error];
                            if (error) {
                                failCallback(error);
                                return;
                            }
                        }
                    }
                                                            
                    [[NSFileManager defaultManager] removeItemAtPath:diffManifestFilePath
                                                                error:&error];
                    if (error) {
                        failCallback(error);
                        return;
                    }
                    */
                }

                unzippedFolder.RenameAsync(newUpdateHash, NameCollisionOption::ReplaceExisting).get();
                newUpdateFolder = unzippedFolder;

                auto relativeBundlePath{ FileUtils::FindFilePathAsync(newUpdateFolder, expectedBundleFileName).get() };
                if (!relativeBundlePath.empty())
                {
                    mutableUpdatePackage.Insert(RelativeBundlePathKey, JsonValue::CreateStringValue(relativeBundlePath));
                }
                else
                {
                    auto errorMessage{ L"" };
                    failCallback(hresult_error{ HRESULT_FROM_WIN32(E_FAIL), errorMessage });
                    return;
                }

                // What on earth is this for?
                /*
                if ([[NSFileManager defaultManager] fileExistsAtPath:newUpdateMetadataPath]) {
                    [[NSFileManager defaultManager] removeItemAtPath:newUpdateMetadataPath
                                                                error:&error];
                    if (error) {
                        failCallback(error);
                        return;
                    }
                }
                */
                
                CodePushUtils::Log((isDiffUpdate) ? L"Applying diff update." : L"Applying full update.");
                auto isSignatureVerificationEnabled{ !publicKey.empty() };

                bool isSignatureAppearedInBundle{ false };
                /*
                NSString *signatureFilePath = [CodePushUpdateUtils getSignatureFilePath:newUpdateFolderPath];
                BOOL isSignatureAppearedInBundle = [[NSFileManager defaultManager] fileExistsAtPath:signatureFilePath];
                */

                if (isSignatureVerificationEnabled)
                {
                    /*
                    if (isSignatureAppearedInBundle) {
                        if (![CodePushUpdateUtils verifyFolderHash:newUpdateFolderPath
                                                        expectedHash:newUpdateHash
                                                                error:&error]) {
                            CPLog(@"The update contents failed the data integrity check.");
                            if (!error) {
                                error = [CodePushErrorUtils errorWithMessage:@"The update contents failed the data integrity check."];
                            }

                            failCallback(error);
                            return;
                        } else {
                            CPLog(@"The update contents succeeded the data integrity check.");
                        }
                        BOOL isSignatureValid = [CodePushUpdateUtils verifyUpdateSignatureFor:newUpdateFolderPath
                                                                                    expectedHash:newUpdateHash
                                                                                withPublicKey:publicKey
                                                                                        error:&error];
                        if (!isSignatureValid) {
                            CPLog(@"The update contents failed code signing check.");
                            if (!error) {
                                error = [CodePushErrorUtils errorWithMessage:@"The update contents failed code signing check."];
                            }
                            failCallback(error);
                            return;
                        } else {
                            CPLog(@"The update contents succeeded the code signing check.");
                        }
                    } else {
                        error = [CodePushErrorUtils errorWithMessage:
                                    @"Error! Public key was provided but there is no JWT signature within app bundle to verify " \
                                    "Possible reasons, why that might happen: \n" \
                                    "1. You've been released CodePush bundle update using version of CodePush CLI that is not support code signing.\n" \
                                    "2. You've been released CodePush bundle update without providing --privateKeyPath option."];
                        failCallback(error);
                        return;
                    }
                    */
                }
                else
                {
                    bool needToVerifyHash;
                    if (isSignatureAppearedInBundle)
                    {
                        CodePushUtils::Log(L"");
                        needToVerifyHash = true;
                    }
                    else
                    {
                        needToVerifyHash = isDiffUpdate;
                    }

                    if (needToVerifyHash)
                    {
                        /*
                        if (![CodePushUpdateUtils verifyFolderHash:newUpdateFolderPath
                                                        expectedHash:newUpdateHash
                                                                error:&error]) {
                            CPLog(@"The update contents failed the data integrity check.");
                            if (!error) {
                                error = [CodePushErrorUtils errorWithMessage:@"The update contents failed the data integrity check."];
                            }

                            failCallback(error);
                            return;
                        } else {
                            CPLog(@"The update contents succeeded the data integrity check.");
                        }
                        */
                    }
                }
            }
            else
            {
                try
                {
                    newUpdateFolder = codePushFolder.CreateFolderAsync(newUpdateHash, CreationCollisionOption::ReplaceExisting).get();
                    auto downloadFile{ codePushFolder.GetFileAsync(DownloadFileName).get() };
                    downloadFile.MoveAsync(newUpdateFolder, UpdateBundleFileName, NameCollisionOption::ReplaceExisting).get();
                }
                catch (const hresult_error& ex)
                {
                    failCallback(ex);
                    return;
                }
            }
            
            newUpdateMetadataFile = newUpdateFolder.CreateFileAsync(UpdateMetadataFileName, CreationCollisionOption::ReplaceExisting).get();

            try
            {
                auto packageJsonString{ mutableUpdatePackage.Stringify() };
                FileIO::WriteTextAsync(newUpdateMetadataFile, packageJsonString);
            }
            catch (const hresult_error& ex)
            {
                failCallback(ex);
            }
            doneCallback();
        }, 
        failCallback };

    downloadHandler.Download(updatePackage.GetNamedString(L"downloadUrl"));

	co_return;
}

StorageFolder GetCodePushFolder()
{
    auto localStorage{ CodePushNativeModule::GetLocalStorageFolder() };
    auto codePushFolder{ localStorage.CreateFolderAsync(L"CodePush", CreationCollisionOption::OpenIfExists).get() };
    if (false /*CodePushNativeModule::IsUsingTestConfiguration()*/) // How can I have this static function refer to a member variable
    {
        return codePushFolder.CreateFolderAsync(L"TestPackages", CreationCollisionOption::OpenIfExists).get();
    }
    return codePushFolder;
}

path GetCodePushPath()
{
    auto codePushPath{ CodePushNativeModule::GetLocalStoragePath() / L"CodePush" };
    if (false /*CodePushNativeModule::IsUsingTestConfiguration()*/) // How can I have this static function refer to a member variable
    {
        codePushPath /= L"TestPackages";
    }
    return codePushPath;
}

IAsyncOperation<JsonObject> CodePushPackage::GetCurrentPackageAsync()
{
    auto packageHash{ co_await GetCurrentPackageHashAsync() };
    if (packageHash.empty())
    {
        co_return nullptr;
    }
	co_return co_await GetPackageAsync(packageHash);
}

IAsyncOperation<hstring> CodePushPackage::GetCurrentPackageHashAsync()
{
    auto info{ co_await GetCurrentPackageInfoAsync() };
    if (info == nullptr)
    {
        co_return L"";
    }
    co_return info.Lookup(L"currentPackage").GetString();
}

IAsyncOperation<JsonObject> GetCurrentPackageInfoAsync()
{
    try
    {
        auto statusFilePath{ GetStatusFilePath() };
        auto parentFolder{ co_await StorageFolder::GetFolderFromPathAsync(statusFilePath.parent_path().wstring()) };
        auto statusFile{ (co_await parentFolder.TryGetItemAsync(statusFilePath.filename().wstring())).try_as<StorageFile>() };
        if (statusFile == nullptr)
        {
            co_return nullptr;
        }
        auto content{ co_await FileIO::ReadTextAsync(statusFile) };
        JsonObject json;
        auto success{ JsonObject::TryParse(content, json) };
        if (!success)
        {
            co_return nullptr;
        }
        co_return json;
    }
    catch (...)
    {
        // Either the file does not exist or does not contain valid JSON
        co_return nullptr;
    }
    co_return nullptr;
}

IAsyncOperation<JsonObject> CodePushPackage::GetPreviousPackageAsync()
{
    auto packageHash{ co_await GetPreviousPackageHashAsync() };
    if (packageHash.empty())
    {
        return nullptr;
    }
    co_return co_await GetPackageAsync(packageHash);
}

IAsyncOperation<hstring> GetPreviousPackageHashAsync()
{
    auto info{ co_await GetCurrentPackageInfoAsync() };
    if (info == nullptr)
    {
        co_return L"";
    }
    co_return info.Lookup(L"previousPackage").GetString();
}

StorageFile GetDownloadFile()
{
    return FileUtils::GetOrCreateFileAsync(GetCodePushFolder(), CodePushPackage::DownloadFileName).get();
}

path GetDownloadFilePath()
{
    return GetCodePushPath() / CodePushPackage::DownloadFileName;
    //return GetDownloadFile().Path().c_str();
}

/*
+ (NSString*)getDownloadFilePath
{
    return [[self getCodePushPath]stringByAppendingPathComponent:DownloadFileName];
}
*/

IAsyncOperation<JsonObject> CodePushPackage::GetPackageAsync(wstring_view packageHash)
{
    try
    {
        auto updateDirectoryPath{ GetPackageFolderPath(packageHash) };
        auto updateMetadataFilePath{ updateDirectoryPath / UpdateMetadataFileName };
        auto updateMetadataFile{ co_await StorageFile::GetFileFromPathAsync(updateMetadataFilePath.wstring()) };
        auto updateMetadataString{ co_await FileIO::ReadTextAsync(updateMetadataFile) };
        auto updateMetadata{ JsonObject::Parse(updateMetadataString) };
        co_return updateMetadata;
    }
    catch (...)
    {
        co_return nullptr;
    }
	co_return nullptr;
}



path CodePushPackage::GetPackageFolderPath(wstring_view packageHash)
{
    return GetCodePushPath() / packageHash;
}

StorageFile GetStatusFile()
{
    auto localStorage{ CodePushNativeModule::GetLocalStorageFolder() };
    return (localStorage.TryGetItemAsync(CodePushPackage::StatusFile).get().try_as<StorageFile>());
}

path GetStatusFilePath()
{
    auto localStorage{ CodePushNativeModule::GetLocalStoragePath() };
    return localStorage / CodePushPackage::StatusFile;
    //return GetStatusFile().Path().c_str();
}

StorageFolder GetUnzippedFolder()
{
    return FileUtils::GetFolderAtPathAsync(GetCodePushFolder(), CodePushPackage::UnzippedFolderName).get();
}

path GetUnzippedFolderPath()
{
    return GetCodePushPath() / CodePushPackage::UnzippedFolderName;
}
/*
+ (NSString*)getUnzippedFolderPath
{
    return [[self getCodePushPath]stringByAppendingPathComponent:UnzippedFolderName];
}
*/