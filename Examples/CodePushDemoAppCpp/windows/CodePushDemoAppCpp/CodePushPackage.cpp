#include "pch.h"

#include "CodePushNativeModule.h"
#include "CodePushPackage.h"
//#include "JSValueAdditions.h"

//#include "NativeModules.h"
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Foundation.h>

#include <filesystem>
#include <functional>

using namespace CodePush;

using namespace winrt;
using namespace Windows::Data::Json;
using namespace Windows::Storage;
using namespace Windows::Foundation;

using namespace std;

const wstring StatusFile{ L"codepush.json" };

IAsyncOperation<JsonObject> GetCurrentPackageInfoAsync();
IAsyncOperation<hstring> GetPreviousPackageHashAsync();
path GetStatusFilePath();

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

path GetCodePushPath()
{
    return nullptr;
}

/*
+ (NSString*)getCodePushPath
{
    NSString* codePushPath = [[CodePush getApplicationSupportDirectory]stringByAppendingPathComponent:@"CodePush"];
    if ([CodePush isUsingTestConfiguration]) {
        codePushPath = [codePushPath stringByAppendingPathComponent : @"TestPackages"];
    }

    return codePushPath;
}
*/

IAsyncOperation<JsonObject> CodePushPackage::GetCurrentPackageAsync()
{
	auto packageHash{ co_await GetCurrentPackageHashAsync() };
	co_return co_await GetPackageAsync(packageHash);
}

/*
+ (NSDictionary *)getCurrentPackage:(NSError **)error
{
    NSString *packageHash = [CodePushPackage getCurrentPackageHash:error];
    if (!packageHash) {
        return nil;
    }

    return [CodePushPackage getPackage:packageHash error:error];
}
*/

IAsyncOperation<hstring> CodePushPackage::GetCurrentPackageHashAsync()
{
    auto info{ co_await GetCurrentPackageInfoAsync() };
    if (info == nullptr)
    {
        co_return L"";
    }
    co_return info.Lookup(L"currentPackage").GetString();
    //co_return nullptr;
}

/*
+ (NSString *)getCurrentPackageHash:(NSError **)error
{
    NSDictionary *info = [self getCurrentPackageInfo:error];
    if (!info) {
        return nil;
    }

    return info[@"currentPackage"];
}
*/

IAsyncOperation<JsonObject> GetCurrentPackageInfoAsync()
{
    try
    {
        auto statusFilePath{ GetStatusFilePath() };
        auto statusFile{ co_await StorageFile::GetFileFromPathAsync(statusFilePath.wstring()) };
        auto content{ co_await FileIO::ReadTextAsync(statusFile) };
        auto json{ JsonObject::Parse(content) };
        co_return json;
    }
    catch (const hresult_error& ex)
    {
        co_return nullptr;
    }
    co_return nullptr;
}

/*
+ (NSMutableDictionary*)getCurrentPackageInfo:(NSError**)error
{
    NSString* statusFilePath = [self getStatusFilePath];
    if (![[NSFileManager defaultManager]fileExistsAtPath:statusFilePath] ) {
        return[NSMutableDictionary dictionary];
    }

    NSString* content = [NSString stringWithContentsOfFile : statusFilePath
        encoding : NSUTF8StringEncoding
        error : error];
    if (!content) {
        return nil;
    }

    NSData* data = [content dataUsingEncoding : NSUTF8StringEncoding];
    NSDictionary* json = [NSJSONSerialization JSONObjectWithData : data
        options : kNilOptions
        error : error];
    if (!json) {
        return nil;
    }

    return[json mutableCopy];
}
*/

IAsyncOperation<JsonObject> CodePushPackage::GetPreviousPackageAsync()
{
    auto packageHash{ co_await GetPreviousPackageHashAsync() };
    co_return co_await GetPackageAsync(packageHash);
}

/*
+ (NSDictionary *)getPreviousPackage:(NSError **)error
{
    NSString *packageHash = [self getPreviousPackageHash:error];
    if (!packageHash) {
        return nil;
    }

    return [CodePushPackage getPackage:packageHash error:error];
}
*/

IAsyncOperation<hstring> GetPreviousPackageHashAsync()
{
    co_return L"";
}

IAsyncOperation<JsonObject> CodePushPackage::GetPackageAsync(wstring_view packageHash)
{
	co_return nullptr;
}

path GetStatusFilePath()
{
    auto localStorage{ CodePushNativeModule::GetLocalStoragePath() };
    return localStorage / StatusFile;
}

/*
+ (NSString*)getStatusFilePath
{
    return [[self getCodePushPath]stringByAppendingPathComponent:StatusFile];
}
*/
