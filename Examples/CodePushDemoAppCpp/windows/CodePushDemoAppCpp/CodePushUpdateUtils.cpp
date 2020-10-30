#include "pch.h"

//#include "CodePush.h"
#include "winrt/Windows.ApplicationModel.h"
#include "winrt/Windows.Security.Cryptography.h"
#include "winrt/Windows.Security.Cryptography.Core.h"
#include "winrt/Windows.Storage.h"
#include "winrt/Windows.Storage.FileProperties.h"
#include "winrt/Windows.Storage.Streams.h"
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Foundation.Collections.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "CodePushUpdateUtils.h"
#include "CodePushUtils.h"

using namespace std;

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace Windows::Security::Cryptography;
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

IAsyncOperation<bool> CodePushUpdateUtils::AddContentsOfFolderToManifestAsync(const StorageFolder& folder, wstring_view pathPrefix, IMap<hstring, hstring>& manifest)
{
    auto folderFiles{ co_await folder.GetItemsAsync() };
    if (folderFiles.Size() == 0)
    {
        co_return false;
    }

    for (const auto& item : folderFiles)
    {
        auto fullFilePath{ item.Path() };
        auto relativePath{ pathPrefix + L"/" + item.Name() };

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
            manifest.Insert(relativePath, fileContentsHash);
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

IAsyncAction CodePushUpdateUtils::AddFileToManifest(const StorageFile& file, IMap<hstring, hstring>& manifest)
{
    auto fileContents{ co_await FileIO::ReadBufferAsync(file) };
    auto fileContentsHash{ ComputeHashForData(fileContents) };
    manifest.Insert(ManifestFolderPrefix + L"/" + file.Name(), fileContentsHash);
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

hstring CodePushUpdateUtils::ComputeFinalHashFromManifest(IMap<hstring, hstring>& manifest)
{
    JsonObject manifestObject;
    for (const auto& pair : manifest)
    {
        manifestObject.Insert(pair.Key(), JsonValue::CreateStringValue(pair.Value()));
    }

    auto manifestString{ manifestObject.Stringify() };

    // I'm not 100% on the correct encoding scheme. It's either LE or BE
    auto manifestData{ CryptographicBuffer::ConvertStringToBinary(manifestString, BinaryStringEncoding::Utf16BE) };
    return ComputeHashForData(manifestData);
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
    auto hash{ algProvider.HashData(inputData) };
    auto output{ CryptographicBuffer::EncodeToHexString(hash) };
    return output;
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
    //JsonArray manifest;
    //vector<wstring> manifest;
    auto manifest{ single_threaded_map<hstring, hstring>() };

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

    co_await AddFileToManifest(binaryBundle, manifest);
    
    auto binaryHash{ ComputeFinalHashFromManifest(manifest) };

    // Cache the hash in user preferences. This assumes that the modified date for the
    // JS bundle changes every time a new bundle is generated by the packager.
    binaryHashDictionary.Insert(binaryModifiedDate, JsonValue::CreateStringValue(binaryHash));
    localSettings.Values().Insert(BinaryHashKey, box_value(binaryHashDictionary.Stringify()));

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

// remove BEGIN / END tags and line breaks from public key string
wstring CodePushUpdateUtils::GetKeyValueFromPublicKeyString(wstring_view publicKeyString)
{
    wstring copy{ publicKeyString };

    wstring_view strings[]{ L"-----BEGIN PUBLIC KEY-----\n", L"-----END PUBLIC KEY-----", L"\n" };
    for (const auto& str : strings)
    {
        while (size_t pos = copy.find(str) != wstring::npos)
        {
            copy.replace(pos, pos + str.size(), L"");
        }
    }
    
    return copy;
}

/*
// remove BEGIN / END tags and line breaks from public key string
+ (NSString*)getKeyValueFromPublicKeyString:(NSString*)publicKeyString
{
    publicKeyString = [publicKeyString stringByReplacingOccurrencesOfString : @"-----BEGIN PUBLIC KEY-----\n"
        withString:@""];
    publicKeyString = [publicKeyString stringByReplacingOccurrencesOfString : @"-----END PUBLIC KEY-----"
        withString:@""];
    publicKeyString = [publicKeyString stringByReplacingOccurrencesOfString : @"\n"
        withString:@""];

    return publicKeyString;
}
*/

IAsyncOperation<bool> CodePushUpdateUtils::VerifyFolderHashAsync(const StorageFolder& finalUpdateFolder, wstring_view expectedHash)
{
    CodePushUtils::Log(L"Verifying hash for folder: " + finalUpdateFolder.Path());

    auto updateContentsManifest{ single_threaded_map<hstring, hstring>() };
    auto result{ co_await AddContentsOfFolderToManifestAsync(finalUpdateFolder, L"", updateContentsManifest) };

    // log manifest

    if (!result)
    {
        co_return false;
    }

    auto updateContentsManifestHash{ ComputeFinalHashFromManifest(updateContentsManifest) };
    if (updateContentsManifestHash.empty())
    {
        co_return false;
    }

    CodePushUtils::Log(L"Expected hash: " + expectedHash + L", actual hash: " + updateContentsManifestHash);

    co_return updateContentsManifestHash == expectedHash;
}

/*
+ (BOOL)verifyFolderHash:(NSString*)finalUpdateFolder
expectedHash : (NSString*)expectedHash
error : (NSError**)error
{
    CPLog(@"Verifying hash for folder path: %@", finalUpdateFolder);

    NSMutableArray* updateContentsManifest = [NSMutableArray array];
    BOOL result = [self addContentsOfFolderToManifest : finalUpdateFolder
        pathPrefix : @""
        manifest:updateContentsManifest
        error : error];

    CPLog(@"Manifest string: %@", updateContentsManifest);

    if (!result) {
        return NO;
    }

    NSString* updateContentsManifestHash = [self computeFinalHashFromManifest : updateContentsManifest
        error : error];
    if (!updateContentsManifestHash) {
        return NO;
    }

    CPLog(@"Expected hash: %@, actual hash: %@", expectedHash, updateContentsManifestHash);

    return[updateContentsManifestHash isEqualToString : expectedHash];
}
*/

JsonObject CodePushUpdateUtils::VerifyAndDecodeJWT(const wstring& jwt, const wstring& publicKey)
{

}

/*
+(NSDictionary*)verifyAndDecodeJWT:(NSString*)jwt
withPublicKey : (NSString*)publicKey
error : (NSError**)error
{
    id <JWTAlgorithmDataHolderProtocol> verifyDataHolder = [JWTAlgorithmRSFamilyDataHolder new].keyExtractorType([JWTCryptoKeyExtractor publicKeyWithPEMBase64].type).algorithmName(@"RS256").secret(publicKey);

    JWTCodingBuilder* verifyBuilder = [JWTDecodingBuilder decodeMessage : jwt].addHolder(verifyDataHolder);
    JWTCodingResultType* verifyResult = verifyBuilder.result;
    if (verifyResult.successResult) {
        return verifyResult.successResult.payload;
    }
    else {
        *error = verifyResult.errorResult.error;
        return nil;
    }
}
*/

IAsyncOperation<bool> CodePushUpdateUtils::VerifyUpdateSignatureForAsync(const StorageFolder& updateFolder, wstring_view newUpdateHash, wstring_view publicKeyString)
{
    CodePushUtils::Log(L"Verifying signature for folder path: " + updateFolder.Path());
    auto publicKey{ GetKeyValueFromPublicKeyString(publicKeyString) };

    hstring signature;
    try
    {
        signature = co_await GetSignatureForAsync(updateFolder);
    }
    catch (const hresult_error& ex)
    {
        CodePushUtils::Log(L"The update could not be verified because no signature was found. " + ex.message());
        throw ex;
    }



    /*
    NSError* payloadDecodingError;
    NSDictionary* envelopedPayload = [self verifyAndDecodeJWT : signature withPublicKey : publicKey error : &payloadDecodingError];
    if (payloadDecodingError) {
        CPLog(@"The update could not be verified because it was not signed by a trusted party. %@", payloadDecodingError);
        *error = payloadDecodingError;
        return false;
    }

    CPLog(@"JWT signature verification succeeded, payload content:  %@", envelopedPayload);

    if (![envelopedPayload objectForKey : @"contentHash"]) {
        CPLog(@"The update could not be verified because the signature did not specify a content hash.");
        return false;
    }

    NSString* contentHash = envelopedPayload[@"contentHash"];

    return[contentHash isEqualToString : newUpdateHash];
    */

    return false;
}

/*
+ (BOOL)verifyUpdateSignatureFor:(NSString*)folderPath
expectedHash : (NSString*)newUpdateHash
withPublicKey : (NSString*)publicKeyString
error : (NSError**)error
{
    NSLog(@"Verifying signature for folder path: %@", folderPath);

    NSString* publicKey = [self getKeyValueFromPublicKeyString : publicKeyString];

    NSError* signatureVerificationError;
    NSString* signature = [self getSignatureFor : folderPath
        error : &signatureVerificationError];
    if (signatureVerificationError) {
        CPLog(@"The update could not be verified because no signature was found. %@", signatureVerificationError);
        *error = signatureVerificationError;
        return false;
    }

    NSError* payloadDecodingError;
    NSDictionary* envelopedPayload = [self verifyAndDecodeJWT : signature withPublicKey : publicKey error : &payloadDecodingError];
    if (payloadDecodingError) {
        CPLog(@"The update could not be verified because it was not signed by a trusted party. %@", payloadDecodingError);
        *error = payloadDecodingError;
        return false;
    }

    CPLog(@"JWT signature verification succeeded, payload content:  %@", envelopedPayload);

    if (![envelopedPayload objectForKey : @"contentHash"]) {
        CPLog(@"The update could not be verified because the signature did not specify a content hash.");
        return false;
    }

    NSString* contentHash = envelopedPayload[@"contentHash"];

    return[contentHash isEqualToString : newUpdateHash];
}
*/


IAsyncOperation<StorageFile> CodePushUpdateUtils::GetSignatureFileAsync(const StorageFolder& rootFolder)
{
    auto manifestFolder{ (co_await rootFolder.TryGetItemAsync(ManifestFolderPrefix)).try_as<StorageFolder>() };
    if (manifestFolder != nullptr)
    {
        auto bundleJWTFile{ (co_await rootFolder.TryGetItemAsync(BundleJWTFile)).try_as<StorageFile>() };
        if (bundleJWTFile != nullptr)
        {
            co_return bundleJWTFile;
        }
    }
    co_return nullptr;
}

IAsyncOperation<hstring> CodePushUpdateUtils::GetSignatureForAsync(const StorageFolder& folder)
{
    auto signatureFile{ co_await GetSignatureFileAsync(folder) };
    if (signatureFile != nullptr)
    {
        co_return co_await FileIO::ReadTextAsync(signatureFile, UnicodeEncoding::Utf8);
    }
    else
    {
        throw hresult_error{ HRESULT_FROM_WIN32(E_FAIL), L"Cannot find signature." };
        co_return L"";
    }
}

/*
+ (NSString*)getSignatureFor:(NSString*)folderPath
error : (NSError**)error
{
    NSString* signatureFilePath = [self getSignatureFilePath : folderPath];
    if ([[NSFileManager defaultManager]fileExistsAtPath:signatureFilePath] ) {
        return[NSString stringWithContentsOfFile : signatureFilePath encoding : NSUTF8StringEncoding error : error];
    }
    else {
        *error = [CodePushErrorUtils errorWithMessage : [NSString stringWithFormat : @"Cannot find signature at %@", signatureFilePath] ];
        return nil;
    }
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