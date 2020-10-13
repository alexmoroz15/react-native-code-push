#include "pch.h"

#include "CodePushDownloadHandler.h"
#include "CodePushNativeModule.h"
#include "CodePushPackage.h"
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
    auto newUpdateHash{ updatePackage.GetNamedString(L"packageHash") };
    auto newUpdateFolderPath{ GetPackageFolderPath(newUpdateHash) };
    auto newUpdateMetadataPath{ newUpdateFolderPath / UpdateMetadataFileName };

    // DeleteIfExists(newUpdateFolder());
    // if (!FolderExists(CodePushPath()))
    // {
    // CreateFolderFromPath(CodePushPath());
    // }

    // There is no function to check if a folder exists.
    try
    {
        auto newUpdateFolder{ co_await StorageFolder::GetFolderFromPathAsync(newUpdateFolderPath.wstring()) };
        co_await newUpdateFolder.DeleteAsync();
    }
    catch (...) {}

    auto codePushFolderExists{ false };
    try
    {
        auto codePushFolder{ co_await StorageFolder::GetFolderFromPathAsync(GetCodePushPath().wstring()) };
        codePushFolderExists = true;
    }
    catch (...) {}
    if (!codePushFolderExists)
    {
        auto codePushPath{ GetCodePushPath() };
        auto localStoragePath{ CodePushNativeModule::GetLocalStoragePath() };
        path relativePath{ codePushPath.wstring().substr(localStoragePath.wstring().size()) };
        stack<path> folderStack{};
        while (relativePath.has_parent_path())
        {
            folderStack.push(relativePath.stem());
            relativePath = relativePath.parent_path();
        }

        auto rootFolder{ CodePushNativeModule::GetLocalStorageFolder() };
        while (!folderStack.empty())
        {
            auto folderName{ folderStack.top().wstring() };
            auto folder{ (co_await rootFolder.TryGetItemAsync(folderName)).try_as<StorageFolder>() };
            if (folder == nullptr)
            {
                rootFolder = co_await rootFolder.CreateFolderAsync(folderName);
            }
            else
            {
                rootFolder = folder;
            }
            folderStack.pop();
        }
    }

    auto downloadFilePath{ GetDownloadFilePath() };
    auto bundleFilePath{ newUpdateFolderPath / UpdateBundleFileName };

    CodePushDownloadHandler downloadHandler{
        downloadFilePath, 
        progressCallback, 
        [=](bool isZip) 
        {
            auto unzippedFolderPath{ GetUnzippedFolderPath() };
            auto mutableUpdatePackage{ updatePackage };
            if (isZip)
            {
                // Delete unzipped folder if exists.

            }
            else
            {
                /*
                [[NSFileManager defaultManager] createDirectoryAtPath:newUpdateFolderPath
                                            withIntermediateDirectories:YES
                                                            attributes:nil
                                                                error:&error];
                [[NSFileManager defaultManager] moveItemAtPath:downloadFilePath
                                                        toPath:bundleFilePath
                                                            error:&error];
                if (error) {
                    failCallback(error);
                    return;
                }
                */
                // CreateFolderAtPath(newUpdateFolderPath);
                /*
                auto downloadFile{ co_await StorageFile::GetFileFromPathAsync(downloadFilePath.wstring()) };
                co_await downloadFile.MoveAsync(bundleFilePath.parent_path().wstring());
                co_await downloadFile.RenameAsync(bundleFilePath.filename());
                */
                try
                {
                    auto downloadFile{ StorageFile::GetFileFromPathAsync(downloadFilePath.wstring()) };
                    StorageFolder newUpdateFolder{ nullptr };
                    try
                    {
                        newUpdateFolder = StorageFolder::GetFolderFromPathAsync(newUpdateFolderPath.wstring()).get();
                    }
                    catch(const hresult_error& ex)
                    {
                        if (ex.code() == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                        {
                            //newUpdateFolder = 
                        }
                    }
                }
                catch(const hresult_error& ex)
                {
                    failCallback(ex);
                    return;
                }
            }

            /*
            NSError *error = nil;
            NSString * unzippedFolderPath = [CodePushPackage getUnzippedFolderPath];
            NSMutableDictionary * mutableUpdatePackage = [updatePackage mutableCopy];
            if (isZip) {
                if ([[NSFileManager defaultManager] fileExistsAtPath:unzippedFolderPath]) {
                    // This removes any unzipped download data that could have been left
                    // uncleared due to a crash or error during the download process.
                    [[NSFileManager defaultManager] removeItemAtPath:unzippedFolderPath
                                                                error:&error];
                    if (error) {
                        failCallback(error);
                        return;
                    }
                }

                NSError *nonFailingError = nil;
                [SSZipArchive unzipFileAtPath:downloadFilePath
                                toDestination:unzippedFolderPath];
                [[NSFileManager defaultManager] removeItemAtPath:downloadFilePath
                                                            error:&nonFailingError];
                if (nonFailingError) {
                    CPLog(@"Error deleting downloaded file: %@", nonFailingError);
                    nonFailingError = nil;
                }

                NSString *diffManifestFilePath = [unzippedFolderPath stringByAppendingPathComponent:DiffManifestFileName];
                BOOL isDiffUpdate = [[NSFileManager defaultManager] fileExistsAtPath:diffManifestFilePath];

                if (isDiffUpdate) {
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
                }

                [CodePushUpdateUtils copyEntriesInFolder:unzippedFolderPath
                                                destFolder:newUpdateFolderPath
                                                    error:&error];
                if (error) {
                    failCallback(error);
                    return;
                }

                [[NSFileManager defaultManager] removeItemAtPath:unzippedFolderPath
                                                            error:&nonFailingError];
                if (nonFailingError) {
                    CPLog(@"Error deleting downloaded file: %@", nonFailingError);
                    nonFailingError = nil;
                }

                NSString *relativeBundlePath = [CodePushUpdateUtils findMainBundleInFolder:newUpdateFolderPath
                                                                            expectedFileName:expectedBundleFileName
                                                                                        error:&error];

                if (error) {
                    failCallback(error);
                    return;
                }

                if (relativeBundlePath) {
                    [mutableUpdatePackage setValue:relativeBundlePath forKey:RelativeBundlePathKey];
                } else {
                    NSString *errorMessage = [NSString stringWithFormat:@"Update is invalid - A JS bundle file named \"%@\" could not be found within the downloaded contents. Please ensure that your app is syncing with the correct deployment and that you are releasing your CodePush updates using the exact same JS bundle file name that was shipped with your app's binary.", expectedBundleFileName];

                    error = [CodePushErrorUtils errorWithMessage:errorMessage];

                    failCallback(error);
                    return;
                }

                if ([[NSFileManager defaultManager] fileExistsAtPath:newUpdateMetadataPath]) {
                    [[NSFileManager defaultManager] removeItemAtPath:newUpdateMetadataPath
                                                                error:&error];
                    if (error) {
                        failCallback(error);
                        return;
                    }
                }

                CPLog((isDiffUpdate) ? @"Applying diff update." : @"Applying full update.");

                BOOL isSignatureVerificationEnabled = (publicKey != nil);

                NSString *signatureFilePath = [CodePushUpdateUtils getSignatureFilePath:newUpdateFolderPath];
                BOOL isSignatureAppearedInBundle = [[NSFileManager defaultManager] fileExistsAtPath:signatureFilePath];

                if (isSignatureVerificationEnabled) {
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

                } else {
                    BOOL needToVerifyHash;
                    if (isSignatureAppearedInBundle) {
                        CPLog(@"Warning! JWT signature exists in codepush update but code integrity check couldn't be performed" \
                                " because there is no public key configured. " \
                                "Please ensure that public key is properly configured within your application.");
                        needToVerifyHash = true;
                    } else {
                        needToVerifyHash = isDiffUpdate;
                    }
                    if(needToVerifyHash){
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
                    }
                }
            } else {
                [[NSFileManager defaultManager] createDirectoryAtPath:newUpdateFolderPath
                                            withIntermediateDirectories:YES
                                                            attributes:nil
                                                                error:&error];
                [[NSFileManager defaultManager] moveItemAtPath:downloadFilePath
                                                        toPath:bundleFilePath
                                                            error:&error];
                if (error) {
                    failCallback(error);
                    return;
                }
            }

            NSData *updateSerializedData = [NSJSONSerialization dataWithJSONObject:mutableUpdatePackage
                                                                            options:0
                                                                                error:&error];
            NSString *packageJsonString = [[NSString alloc] initWithData:updateSerializedData
                                                                encoding:NSUTF8StringEncoding];

            [packageJsonString writeToFile:newUpdateMetadataPath
                                atomically:YES
                                    encoding:NSUTF8StringEncoding
                                        error:&error];
            if (error) {
                failCallback(error);
            } else {
                doneCallback();
            }
            */
        }, 
        failCallback };

    downloadHandler.Download(updatePackage.GetNamedString(L"downloadUrl"));

	co_return;
}

/*
+ (void)downloadPackage:(NSDictionary *)updatePackage
 expectedBundleFileName:(NSString *)expectedBundleFileName
              publicKey:(NSString *)publicKey
         operationQueue:(dispatch_queue_t)operationQueue
       progressCallback:(void (^)(long long, long long))progressCallback
           doneCallback:(void (^)())doneCallback
           failCallback:(void (^)(NSError *err))failCallback
{
    NSString *newUpdateHash = updatePackage[@"packageHash"];
    NSString *newUpdateFolderPath = [self getPackageFolderPath:newUpdateHash];
    NSString *newUpdateMetadataPath = [newUpdateFolderPath stringByAppendingPathComponent:UpdateMetadataFileName];
    NSError *error;

    if ([[NSFileManager defaultManager] fileExistsAtPath:newUpdateFolderPath]) {
        // This removes any stale data in newUpdateFolderPath that could have been left
        // uncleared due to a crash or error during the download or install process.
        [[NSFileManager defaultManager] removeItemAtPath:newUpdateFolderPath
                                                   error:&error];
    } else if (![[NSFileManager defaultManager] fileExistsAtPath:[self getCodePushPath]]) {
        [[NSFileManager defaultManager] createDirectoryAtPath:[self getCodePushPath]
                                  withIntermediateDirectories:YES
                                                   attributes:nil
                                                        error:&error];

        // Ensure that none of the CodePush updates we store on disk are
        // ever included in the end users iTunes and/or iCloud backups
        NSURL *codePushURL = [NSURL fileURLWithPath:[self getCodePushPath]];
        [codePushURL setResourceValue:@YES forKey:NSURLIsExcludedFromBackupKey error:nil];
    }

    if (error) {
        return failCallback(error);
    }

    NSString *downloadFilePath = [self getDownloadFilePath];
    NSString *bundleFilePath = [newUpdateFolderPath stringByAppendingPathComponent:UpdateBundleFileName];

    CodePushDownloadHandler *downloadHandler = [[CodePushDownloadHandler alloc]
                                                init:downloadFilePath
                                                operationQueue:operationQueue
                                                progressCallback:progressCallback
                                                doneCallback:^(BOOL isZip) {
                                                    NSError *error = nil;
                                                    NSString * unzippedFolderPath = [CodePushPackage getUnzippedFolderPath];
                                                    NSMutableDictionary * mutableUpdatePackage = [updatePackage mutableCopy];
                                                    if (isZip) {
                                                        if ([[NSFileManager defaultManager] fileExistsAtPath:unzippedFolderPath]) {
                                                            // This removes any unzipped download data that could have been left
                                                            // uncleared due to a crash or error during the download process.
                                                            [[NSFileManager defaultManager] removeItemAtPath:unzippedFolderPath
                                                                                                       error:&error];
                                                            if (error) {
                                                                failCallback(error);
                                                                return;
                                                            }
                                                        }

                                                        NSError *nonFailingError = nil;
                                                        [SSZipArchive unzipFileAtPath:downloadFilePath
                                                                        toDestination:unzippedFolderPath];
                                                        [[NSFileManager defaultManager] removeItemAtPath:downloadFilePath
                                                                                                   error:&nonFailingError];
                                                        if (nonFailingError) {
                                                            CPLog(@"Error deleting downloaded file: %@", nonFailingError);
                                                            nonFailingError = nil;
                                                        }

                                                        NSString *diffManifestFilePath = [unzippedFolderPath stringByAppendingPathComponent:DiffManifestFileName];
                                                        BOOL isDiffUpdate = [[NSFileManager defaultManager] fileExistsAtPath:diffManifestFilePath];

                                                        if (isDiffUpdate) {
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
                                                        }

                                                        [CodePushUpdateUtils copyEntriesInFolder:unzippedFolderPath
                                                                                      destFolder:newUpdateFolderPath
                                                                                           error:&error];
                                                        if (error) {
                                                            failCallback(error);
                                                            return;
                                                        }

                                                        [[NSFileManager defaultManager] removeItemAtPath:unzippedFolderPath
                                                                                                   error:&nonFailingError];
                                                        if (nonFailingError) {
                                                            CPLog(@"Error deleting downloaded file: %@", nonFailingError);
                                                            nonFailingError = nil;
                                                        }

                                                        NSString *relativeBundlePath = [CodePushUpdateUtils findMainBundleInFolder:newUpdateFolderPath
                                                                                                                  expectedFileName:expectedBundleFileName
                                                                                                                             error:&error];

                                                        if (error) {
                                                            failCallback(error);
                                                            return;
                                                        }

                                                        if (relativeBundlePath) {
                                                            [mutableUpdatePackage setValue:relativeBundlePath forKey:RelativeBundlePathKey];
                                                        } else {
                                                            NSString *errorMessage = [NSString stringWithFormat:@"Update is invalid - A JS bundle file named \"%@\" could not be found within the downloaded contents. Please ensure that your app is syncing with the correct deployment and that you are releasing your CodePush updates using the exact same JS bundle file name that was shipped with your app's binary.", expectedBundleFileName];

                                                            error = [CodePushErrorUtils errorWithMessage:errorMessage];

                                                            failCallback(error);
                                                            return;
                                                        }

                                                        if ([[NSFileManager defaultManager] fileExistsAtPath:newUpdateMetadataPath]) {
                                                            [[NSFileManager defaultManager] removeItemAtPath:newUpdateMetadataPath
                                                                                                       error:&error];
                                                            if (error) {
                                                                failCallback(error);
                                                                return;
                                                            }
                                                        }

                                                        CPLog((isDiffUpdate) ? @"Applying diff update." : @"Applying full update.");

                                                        BOOL isSignatureVerificationEnabled = (publicKey != nil);

                                                        NSString *signatureFilePath = [CodePushUpdateUtils getSignatureFilePath:newUpdateFolderPath];
                                                        BOOL isSignatureAppearedInBundle = [[NSFileManager defaultManager] fileExistsAtPath:signatureFilePath];

                                                        if (isSignatureVerificationEnabled) {
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

                                                        } else {
                                                            BOOL needToVerifyHash;
                                                            if (isSignatureAppearedInBundle) {
                                                                CPLog(@"Warning! JWT signature exists in codepush update but code integrity check couldn't be performed" \
                                                                      " because there is no public key configured. " \
                                                                      "Please ensure that public key is properly configured within your application.");
                                                                needToVerifyHash = true;
                                                            } else {
                                                                needToVerifyHash = isDiffUpdate;
                                                            }
                                                            if(needToVerifyHash){
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
                                                            }
                                                        }
                                                    } else {
                                                        [[NSFileManager defaultManager] createDirectoryAtPath:newUpdateFolderPath
                                                                                  withIntermediateDirectories:YES
                                                                                                   attributes:nil
                                                                                                        error:&error];
                                                        [[NSFileManager defaultManager] moveItemAtPath:downloadFilePath
                                                                                                toPath:bundleFilePath
                                                                                                 error:&error];
                                                        if (error) {
                                                            failCallback(error);
                                                            return;
                                                        }
                                                    }

                                                    NSData *updateSerializedData = [NSJSONSerialization dataWithJSONObject:mutableUpdatePackage
                                                                                                                   options:0
                                                                                                                     error:&error];
                                                    NSString *packageJsonString = [[NSString alloc] initWithData:updateSerializedData
                                                                                                        encoding:NSUTF8StringEncoding];

                                                    [packageJsonString writeToFile:newUpdateMetadataPath
                                                                        atomically:YES
                                                                          encoding:NSUTF8StringEncoding
                                                                             error:&error];
                                                    if (error) {
                                                        failCallback(error);
                                                    } else {
                                                        doneCallback();
                                                    }
                                                }

                                                failCallback:failCallback];

    [downloadHandler download:updatePackage[@"downloadUrl"]];
}
*/

StorageFolder GetCodePushFolder()
{
    /*
    auto localStorage{ CodePushNativeModule::GetLocalStorageFolder() };
    if (false)
    {
        localStorage = FileUtils::GetOrCreateFolderAsync(localStorage, L"TestPackages").get();
    }
    return FileUtils::GetOrCreateFolderAsync(localStorage, L"CodePush").get();
    */
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

path GetDownloadFilePath()
{
    return GetCodePushPath() / CodePushPackage::DownloadFileName;
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

path GetStatusFilePath()
{
    auto localStorage{ CodePushNativeModule::GetLocalStoragePath() };
    return localStorage / CodePushPackage::StatusFile;
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