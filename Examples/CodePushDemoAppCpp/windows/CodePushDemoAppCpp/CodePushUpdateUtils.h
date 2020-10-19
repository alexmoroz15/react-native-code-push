#pragma once

#include "winrt/Windows.Data.Json.h"
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Storage.h"
#include <string>
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
    using namespace Windows::Storage;

    using namespace std;
    using namespace filesystem;

    struct CodePushUpdateUtils
    {
    private:
        inline static const wstring_view AssetsFolderName{ L"assets" };
        inline static const wstring_view BinaryHashKey{ L"CodePushBinaryHash" };
        inline static const wstring_view ManifestFolderPrefix{ L"CodePush" };
        inline static const wstring_view BundleJWTFile{ L".codepushrelease" };

        /*
         Ignore list for hashing
         */
        inline static const wstring_view IgnoreMacOSX{ L"__MACOSX/" };
        inline static const wstring_view IgnoreDSStore{ L".DS_Store" };
        inline static const wstring_view IgnoreCodePushMetadata{ L".codepushrelease" };

    public:
        static bool CopyEntriesInFolder(StorageFolder& sourceFolder, StorageFolder& destFolder);
        static StorageFile FildMainBundleInFolder(StorageFolder& folderPath, const wstring& expectedFileName);
        static IAsyncOperation<hstring> GetHashForBinaryContents(const StorageFile& binaryBundle);
        static IAsyncOperation<hstring> ModifiedDateStringOfFileAsync(const StorageFile& file);
        static bool IsHashIgnoredFor(path relativePath);
        static bool VerifyFolderHash(StorageFolder& finalUpdateFolder, const wstring& expectedHash);

        static wstring GetKeyValueFromPublicKeyString(const wstring& publicKeyString);
        static wstring GetSignatureFilePath(const path& updateFolderPath);
        static JsonObject VerifyAndDecodeJWT(const wstring& jwt, const wstring& publicKey);
        static bool VerifyUpdateSignatureFor(const path& updateFolderPath, const wstring& newUpdateHash, const wstring& publicKeyString);
    };
}