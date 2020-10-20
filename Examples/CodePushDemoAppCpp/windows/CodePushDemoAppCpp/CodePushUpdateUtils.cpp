#include "pch.h"

//#include "CodePush.h"
#include "winrt/Windows.ApplicationModel.h"
#include "winrt/Windows.Security.Cryptography.Core.h"
#include "winrt/Windows.Storage.h"
#include "winrt/Windows.Storage.FileProperties.h"
#include "winrt/Windows.Storage.Streams.h"
#include "winrt/Windows.Foundation.h"

#include <string>
#include <string_view>

#include "CodePushUpdateUtils.h"

using namespace std;

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Storage;
using namespace Windows::Security::Cryptography::Core;

using namespace CodePush;

bool CodePushUpdateUtils::IsHashIgnoredFor(wstring_view relativePath)
{
    return relativePath.find(IgnoreMacOSX) != wstring_view::npos ||
        relativePath.rfind(IgnoreDSStore) != wstring_view::npos ||
        relativePath.rfind(IgnoreCodePushMetadata) != wstring_view::npos;
}
/*
+(BOOL)isHashIgnoredFor:(NSString*)relativePath
{
    return[relativePath hasPrefix : IgnoreMacOSX]
        || [relativePath isEqualToString : IgnoreDSStore]
        || [relativePath hasSuffix : [NSString stringWithFormat : @"/%@", IgnoreDSStore] ]
        || [relativePath isEqualToString : IgnoreCodePushMetadata]
        || [relativePath hasSuffix : [NSString stringWithFormat : @"/%@", IgnoreCodePushMetadata] ];
}
*/

IAsyncOperation<bool> CodePushUpdateUtils::AddContentsOfFolderToManifestAsync(const StorageFolder& folder, wstring_view pathPrefix, JsonArray& manifest)
{
    auto folderFiles{ co_await folder.GetItemsAsync() };
    if (folderFiles.Size() == 0)
    {
        co_return false;
    }

    for (const auto& item : folderFiles)
    {
        auto fullFilePath{ item.Path() };
        auto relativePath{ pathPrefix + item.Name() };

        if (IsHashIgnoredFor(relativePath))
        {
            continue;
        }

        auto isDir{ item.IsOfType(StorageItemTypes::Folder) };
        if (isDir)
        {
            auto result{ co_await AddContentsOfFolderToManifestAsync(item.try_as<StorageFolder>(), relativePath, manifest) };
            if (!result)
            {
                co_return false;
            }
        }
        else
        {
            auto fileContents{ co_await FileIO::ReadBufferAsync(item.try_as<StorageFile>()) };
            auto fileContentsHash{ ComputeHashForData(fileContents) };
            manifest.Append(JsonValue::CreateStringValue(relativePath + L":" + fileContentsHash));
        }
    }

    co_return true;
}

/*
+(BOOL)addContentsOfFolderToManifest:(NSString*)folderPath
pathPrefix : (NSString*)pathPrefix
manifest : (NSMutableArray*)manifest
error : (NSError**)error
{
    NSArray* folderFiles = [[NSFileManager defaultManager]
        contentsOfDirectoryAtPath:folderPath
        error : error];
    if (!folderFiles) {
        return NO;
    }

    for (NSString* fileName in folderFiles) {

        NSString* fullFilePath = [folderPath stringByAppendingPathComponent : fileName];
        NSString* relativePath = [pathPrefix stringByAppendingPathComponent : fileName];

        if ([self isHashIgnoredFor : relativePath]) {
            continue;
        }

        BOOL isDir = NO;
        if ([[NSFileManager defaultManager]fileExistsAtPath:fullFilePath
            isDirectory : &isDir] && isDir) {
            BOOL result = [self addContentsOfFolderToManifest : fullFilePath
                pathPrefix : relativePath
                manifest : manifest
                error : error];
            if (!result) {
                return NO;
            }
        }
        else {
            NSData* fileContents = [NSData dataWithContentsOfFile : fullFilePath];
            NSString* fileContentsHash = [self computeHashForData : fileContents];
            [manifest addObject : [[relativePath stringByAppendingString : @":"]stringByAppendingString:fileContentsHash] ] ;
        }
    }

    return YES;
}
*/

IAsyncAction CodePushUpdateUtils::AddFileToManifest(const StorageFile& file, JsonArray& manifest)
{
    co_return;
}

/*
+(void)addFileToManifest:(NSURL*)fileURL
manifest : (NSMutableArray*)manifest
{
    if ([[NSFileManager defaultManager]fileExistsAtPath:[fileURL path] ] ) {
        NSData* fileContents = [NSData dataWithContentsOfURL : fileURL];
        NSString* fileContentsHash = [self computeHashForData : fileContents];
        [manifest addObject : [NSString stringWithFormat : @"%@/%@:%@", [self manifestFolderPrefix], [fileURL lastPathComponent], fileContentsHash] ] ;
    }
}
*/

hstring CodePushUpdateUtils::ComputeFinalHashFromManifest(const JsonArray& manifest)
{
    return L"";
}

/*
+ (NSString*)computeFinalHashFromManifest:(NSMutableArray*)manifest
error : (NSError**)error
{
    //sort manifest strings to make sure, that they are completely equal with manifest strings has been generated in cli!
    NSArray* sortedManifest = [manifest sortedArrayUsingSelector : @selector(compare:)];
    NSData* manifestData = [NSJSONSerialization dataWithJSONObject : sortedManifest
        options : kNilOptions
        error : error];
    if (!manifestData) {
        return nil;
    }

    NSString* manifestString = [[NSString alloc]initWithData:manifestData
        encoding : NSUTF8StringEncoding];
    // The JSON serialization turns path separators into "\/", e.g. "CodePush\/assets\/image.png"
    manifestString = [manifestString stringByReplacingOccurrencesOfString : @"\\/"
        withString:@"/"];
    return[self computeHashForData : [NSData dataWithBytes : manifestString.UTF8String length : manifestString.length] ];
}
*/

hstring CodePushUpdateUtils::ComputeHashForData(const IBuffer& inputData)
{
    auto algProvider{ HashAlgorithmProvider::OpenAlgorithm(HashAlgorithmNames::Sha256()) };
    auto hash{ algProvider.CreateHash() };
    return L"";
}

/*
+(NSString*)computeHashForData:(NSData*)inputData
{
    uint8_t digest[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256(inputData.bytes, (CC_LONG)inputData.length, digest);
    NSMutableString* inputHash = [NSMutableString stringWithCapacity : CC_SHA256_DIGEST_LENGTH * 2];
    for (int i = 0; i < CC_SHA256_DIGEST_LENGTH; i++) {
        [inputHash appendFormat : @"%02x", digest[i]] ;
    }

    return inputHash;
}
*/

IAsyncOperation<hstring> CodePushUpdateUtils::GetHashForBinaryContents(const StorageFile& binaryBundle)
{
    // Get the cached hash from local settings if it exists
    auto binaryModifiedDate{ co_await ModifiedDateStringOfFileAsync(binaryBundle) };
    auto localSettings{ ApplicationData::Current().LocalSettings() };
    auto binaryHashDictionaryData{ localSettings.Values().TryLookup(BinaryHashKey) };
    JsonObject binaryHashDictionary;
    if (binaryHashDictionaryData != nullptr)
    {
        auto binaryHashDictionaryString{ unbox_value<hstring>(binaryHashDictionaryData) };
        auto success{ JsonObject::TryParse(binaryHashDictionaryString, binaryHashDictionary) };
        if (success)
        {
            auto binaryHash{ binaryHashDictionary.GetNamedString(binaryModifiedDate, L"") };
            if (binaryHash.empty())
            {
                localSettings.Values().Remove(BinaryHashKey);
            }
            else
            {
                co_return binaryHash;
            }
        }
    }

    JsonObject dictionary;
    JsonArray manifest;

    // If the app is using assets, then add
    // them to the generated content manifest.
    auto binaryFolder{ Windows::ApplicationModel::Package::Current().InstalledLocation() };
    auto bundleFolder{ (co_await binaryFolder.TryGetItemAsync(L"bundle")).try_as<StorageFolder>() };
    if (bundleFolder != nullptr)
    {
        auto assetsFolder{ (co_await bundleFolder.TryGetItemAsync(L"assets")).try_as<StorageFolder>() };
        if (assetsFolder != nullptr)
        {
            auto result{ co_await AddContentsOfFolderToManifestAsync(
                assetsFolder,
                L"CodePush/assets",
                manifest) };

            if (!result)
            {
                co_return L"";
            }
        }
    }

    AddFileToManifest(binaryBundle, manifest);
    
    auto binaryHash{ ComputeFinalHashFromManifest(manifest) };

    // Cache the hash in user preferences. This assumes that the modified date for the
    // JS bundle changes every time a new bundle is generated by the packager.
    binaryHashDictionary.Insert(binaryModifiedDate, JsonValue::CreateStringValue(binaryHash));
    localSettings.Values().Insert(BinaryHashKey, binaryHashDictionary);

    co_return binaryHash;
}

/*
+ (NSString*)getHashForBinaryContents:(NSURL*)binaryBundleUrl
error : (NSError**)error
{
    // Get the cached hash from user preferences if it exists.
    NSString* binaryModifiedDate = [self modifiedDateStringOfFileAtURL : binaryBundleUrl];
    NSUserDefaults* preferences = [NSUserDefaults standardUserDefaults];
    NSMutableDictionary* binaryHashDictionary = [preferences objectForKey : BinaryHashKey];
    NSString* binaryHash = nil;
    if (binaryHashDictionary != nil) {
        binaryHash = [binaryHashDictionary objectForKey : binaryModifiedDate];
        if (binaryHash == nil) {
            [preferences removeObjectForKey : BinaryHashKey] ;
            [preferences synchronize] ;
        }
        else {
            return binaryHash;
        }
    }

    binaryHashDictionary = [NSMutableDictionary dictionary];
    NSMutableArray* manifest = [NSMutableArray array];

    // If the app is using assets, then add
    // them to the generated content manifest.
    NSString* assetsPath = [CodePush bundleAssetsPath];
    if ([[NSFileManager defaultManager]fileExistsAtPath:assetsPath] ) {

        BOOL result = [self addContentsOfFolderToManifest : assetsPath
            pathPrefix : [NSString stringWithFormat : @"%@/%@", [self manifestFolderPrefix], @"assets"]
            manifest : manifest
            error : error];
        if (!result) {
            return nil;
        }
    }

    [self addFileToManifest : binaryBundleUrl manifest : manifest] ;
    [self addFileToManifest : [binaryBundleUrl URLByAppendingPathExtension : @"meta"] manifest : manifest] ;

    binaryHash = [self computeFinalHashFromManifest : manifest error : error];

    // Cache the hash in user preferences. This assumes that the modified date for the
    // JS bundle changes every time a new bundle is generated by the packager.
    [binaryHashDictionary setObject : binaryHash forKey : binaryModifiedDate];
    [preferences setObject : binaryHashDictionary forKey : BinaryHashKey] ;
    [preferences synchronize] ;
    return binaryHash;
}
*/

IAsyncOperation<hstring> CodePushUpdateUtils::ModifiedDateStringOfFileAsync(const StorageFile& file)
{
    auto basicProperties{ co_await file.GetBasicPropertiesAsync() };
    auto modifiedDate{ basicProperties.DateModified() };
    auto mtime{ clock::to_time_t(modifiedDate) };
    auto modifiedDateString{ to_hstring(mtime) };
	co_return modifiedDateString;
}