#pragma once

#include "winrt/Windows.Data.Json.h"
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Foundation.Collections.h"
#include "winrt/Windows.Storage.h"
#include "winrt/Windows.Storage.Streams.h"

#include <string>
#include <string_view>
#include <filesystem>

/*
@interface CodePushUpdateUtils : NSObject

+ (BOOL)copyEntriesInFolder:(NSString *)sourceFolder
                 destFolder:(NSString *)destFolder
                      error:(NSError **)error;

+ (NSString *)findMainBundleInFolder:(NSString *)folderPath
                    expectedFileName:(NSString *)expectedFileName
                               error:(NSError **)error;

+ (NSString *)assetsFolderName;
+ (NSString *)getHashForBinaryContents:(NSURL *)binaryBundleUrl
                                 error:(NSError **)error;

+ (NSString *)manifestFolderPrefix;
+ (NSString *)modifiedDateStringOfFileAtURL:(NSURL *)fileURL;

+ (BOOL)isHashIgnoredFor:(NSString *) relativePath;

+ (BOOL)verifyFolderHash:(NSString *)finalUpdateFolder
                   expectedHash:(NSString *)expectedHash
                          error:(NSError **)error;

// remove BEGIN / END tags and line breaks from public key string
+ (NSString *)getKeyValueFromPublicKeyString:(NSString *)publicKeyString;

+ (NSString *)getSignatureFilePath:(NSString *)updateFolderPath;

+ (NSDictionary *) verifyAndDecodeJWT:(NSString *) jwt
               withPublicKey:(NSString *)publicKey
                       error:(NSError **)error;

+ (BOOL)verifyUpdateSignatureFor:(NSString *)updateFolderPath
                    expectedHash:(NSString *)newUpdateHash
                   withPublicKey:(NSString *)publicKeyString
                           error:(NSError **)error;

@end
*/

namespace CodePush
{
    using namespace winrt;
    using namespace Windows::Data::Json;
    using namespace Windows::Foundation;
    using namespace Windows::Foundation::Collections;
    using namespace Windows::Storage;
    using namespace Windows::Storage::Streams;

    using namespace std;
    using namespace filesystem;

    struct CodePushUpdateUtils
    {
    private:
        
        /*
         Ignore list for hashing
         */
        inline static const wstring_view IgnoreMacOSX{ L"__MACOSX/" };
        inline static const wstring_view IgnoreDSStore{ L".DS_Store" };
        inline static const wstring_view IgnoreCodePushMetadata{ L".codepushrelease" };

        static bool IsHashIgnoredFor(wstring_view relativePath);
        static IAsyncOperation<bool> AddContentsOfFolderToManifestAsync(const StorageFolder& folder, wstring_view pathPrefix, IMap<hstring, hstring>& manifest);
        static IAsyncAction AddFileToManifest(const StorageFile& file, IMap<hstring, hstring>& manifest);
        static hstring ComputeFinalHashFromManifest(IMap<hstring, hstring>& manifest);
        static hstring ComputeHashForData(const IBuffer& inputData);
        static IAsyncOperation<hstring> GetSignatureForAsync(const StorageFolder& folder);

    public:
        inline static const wstring_view AssetsFolderName{ L"assets" };
        inline static const wstring_view BinaryHashKey{ L"CodePushBinaryHash" };
        inline static const wstring_view ManifestFolderPrefix{ L"CodePush" };
        inline static const wstring_view BundleJWTFile{ L".codepushrelease" };

        static IAsyncOperation<bool> CopyEntriesInFolderAsync(StorageFolder& sourceFolder, StorageFolder& destFolder);
        static StorageFile FildMainBundleInFolder(StorageFolder& folderPath, const wstring& expectedFileName);
        static IAsyncOperation<hstring> GetHashForBinaryContents(const StorageFile& binaryBundle);
        static IAsyncOperation<hstring> ModifiedDateStringOfFileAsync(const StorageFile& file);
        static bool IsHashIgnoredFor(path relativePath);
        static IAsyncOperation<bool> VerifyFolderHashAsync(const StorageFolder& finalUpdateFolder, wstring_view expectedHash);

        static wstring GetKeyValueFromPublicKeyString(wstring_view publicKeyString);
        static IAsyncOperation<StorageFile> GetSignatureFileAsync(const StorageFolder& rootFolder);
        static JsonObject VerifyAndDecodeJWT(const wstring& jwt, const wstring& publicKey);
        static IAsyncOperation<bool> VerifyUpdateSignatureForAsync(const StorageFolder& updateFolder, wstring_view newUpdateHash, wstring_view publicKeyString);
    };
}