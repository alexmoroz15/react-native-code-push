#include "pch.h"

#include "CodePush.h"
#include "CodePushUtils.h"
#include "CodePushUpdateUtils.h"
#include "CodePushPackage.h"
#include "CodePushTelemetryManager.h"
#include "CodePushConfig.h"
#include "App.h"

#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.Web.Http.h>
#include <winrt/Windows.Web.Http.Headers.h>
#include <winrt/Windows.Storage.Streams.h>

#include <exception>
#include <filesystem>
#include <cstdio>
#include <stack>
#include <string_view>

#include "miniz.h"

#include "AutolinkedNativeModules.g.h"
#include "ReactPackageProvider.h"

using namespace winrt;
using namespace Microsoft::ReactNative;
using namespace Windows::Data::Json;
using namespace Windows::Web::Http;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::Foundation;

using namespace std::filesystem;

path CodePush::CodePush::GetBinaryBundlePath() { return nullptr; }

IAsyncOperation<StorageFile> CodePush::CodePush::GetBundleFileAsync() { co_return nullptr; }
path CodePush::CodePush::GetBundlePath() { return nullptr; }

void CodePush::CodePush::OverrideAppVersion(wstring appVersion) {}
void CodePush::CodePush::SetDeploymentKey(wstring deploymentKey) {}

bool CodePush::CodePush::IsFailedHash(wstring packageHash) { return false; }

JsonObject CodePush::CodePush::GetRollbackInfo() { return nullptr; }
//void SetLatestRollbackInfo(wstring packageHash);
int CodePush::CodePush::GetRollbackCountForPackage(wstring packageHash, JsonObject latestRollbackInfo) { return 0; }

bool CodePush::CodePush::IsPendingUpdate(wstring packageHash) { return false; }

bool CodePush::CodePush::IsUsingTestConfiguration() { return false; }
void CodePush::CodePush::SetUsingTestConfiguration() {}
//void ClearUpdates();

void CodePush::CodePush::LoadBundle()
{
    m_host.InstanceSettings().UIDispatcher().Post([host = m_host]() {
        host.ReloadInstance();
        });
}

/*
 * This method is used to register the fact that a pending
 * update succeeded and therefore can be removed.
 */
void CodePush::CodePush::RemovePendingUpdate()
{
    // remove pending update from LocalSettings
}

/*
+ (void)removePendingUpdate
{
    NSUserDefaults *preferences = [NSUserDefaults standardUserDefaults];
    [preferences removeObjectForKey:PendingUpdateKey];
    [preferences synchronize];
}
*/

void CodePush::CodePush::RestartAppInternal(bool onlyIfUpdateIsPending)
{
    if (m_restartInProgress)
    {
        CodePushUtils::Log(L"Restart request queued until the current restart is completed.");
        m_restartQueue.push_back(onlyIfUpdateIsPending);
    }
    else if (!m_allowed)
    {
        CodePushUtils::Log(L"Restart request queued until restarts are re-allowed.");
        m_restartQueue.push_back(onlyIfUpdateIsPending);
        return;
    }

    m_restartInProgress = true;
    if (!onlyIfUpdateIsPending || IsPendingUpdate(nullptr))
    {
        LoadBundle();
        CodePushUtils::Log(L"Restarting app.");
        return;
    }

    m_restartInProgress = false;
    if (m_restartQueue.size() > 0)
    {
        auto buf{ m_restartQueue[0] };
        m_restartQueue.erase(m_restartQueue.begin());
        RestartAppInternal(buf);
    }
}

void CodePush::CodePush::Initialize(ReactContext const& reactContext) noexcept
{
    m_context = reactContext;

    auto res = reactContext.Properties().Handle().Get(ReactPropertyBagHelper::GetName(nullptr, L"MyReactNativeHost"));
    m_host = res.as<ReactNativeHost>();
}

void CodePush::CodePush::GetConstants(winrt::Microsoft::ReactNative::ReactConstantProvider& constants) noexcept
{
    constants.Add(L"codePushInstallModeImmediate", CodePushInstallMode::IMMEDIATE);
    constants.Add(L"codePushInstallModeOnNextRestart", CodePushInstallMode::ON_NEXT_RESTART);
    constants.Add(L"codePushInstallModeOnNextResume", CodePushInstallMode::ON_NEXT_RESUME);
    constants.Add(L"codePushInstallModeOnNextSuspend", CodePushInstallMode::ON_NEXT_SUSPEND);

    constants.Add(L"codePushUpdateStateRunning", CodePushUpdateState::RUNNING);
    constants.Add(L"codePushUpdateStatePending", CodePushUpdateState::PENDING);
    constants.Add(L"codePushUpdateStateLatest", CodePushUpdateState::LATEST);
}

IAsyncOperation<StorageFile> CreateFileFromPathAsync(StorageFolder rootFolder, path& relPath)
{
    std::stack<std::string> pathParts;
    pathParts.push(relPath.filename().string());
    while (relPath.has_parent_path())
    {
        relPath = relPath.parent_path();
        pathParts.push(relPath.filename().string());
    }

    while (pathParts.size() > 1)
    {
        auto itemName{ pathParts.top() };
        auto item{ co_await rootFolder.TryGetItemAsync(to_hstring(itemName)) };
        if (item == nullptr)
        {
            rootFolder = co_await rootFolder.CreateFolderAsync(to_hstring(itemName));
        }
        else
        {
            rootFolder = item.try_as<StorageFolder>();
        }
        pathParts.pop();
    }
    auto fileName{ pathParts.top() };
    auto file{ co_await rootFolder.CreateFileAsync(to_hstring(fileName), CreationCollisionOption::ReplaceExisting) };
    co_return file;
}

IAsyncOperation<hstring> FindFilePathAsync(const StorageFolder& rootFolder, std::wstring_view fileName)
{
    std::stack<StorageFolder> candidateFolders;
    candidateFolders.push(rootFolder);

    while (!candidateFolders.empty())
    {
        auto relRootFolder = candidateFolders.top();
        candidateFolders.pop();

        auto files{ co_await relRootFolder.GetFilesAsync() };
        for (const auto& file : files)
        {
            if (file.Name() == fileName)
            {
                std::wstring filePath{ file.Path() };
                hstring filePathSub{ filePath.substr(rootFolder.Path().size() + 1) };
                co_return filePathSub;
            }
        }

        auto folders{ co_await rootFolder.GetFoldersAsync() };
        for (const auto& folder : folders)
        {
            candidateFolders.push(folder);
        }
    }

    co_return L"";
}

IAsyncAction UnzipAsync(StorageFile& zipFile, StorageFolder& destination)
{
    std::string zipName{ to_string(zipFile.Path()) };

    mz_bool status;
    mz_zip_archive zip_archive;
    mz_zip_zero_struct(&zip_archive);

    status = mz_zip_reader_init_file(&zip_archive, zipName.c_str(), 0);
    assert(status);
    auto numFiles{ mz_zip_reader_get_num_files(&zip_archive) };

    for (mz_uint i = 0; i < numFiles; i++)
    {
        mz_zip_archive_file_stat file_stat;
        status = mz_zip_reader_file_stat(&zip_archive, i, &file_stat);
        assert(status);
        if (!mz_zip_reader_is_file_a_directory(&zip_archive, i))
        {
            auto fileName{ file_stat.m_filename };
            auto filePath{ path(fileName) };
            auto filePathName{ filePath.filename() };
            auto filePathNameString{ filePathName.string() };

            auto entryFile{ co_await CreateFileFromPathAsync(destination, filePath) };
            auto stream{ co_await entryFile.OpenAsync(FileAccessMode::ReadWrite) };
            auto os{ stream.GetOutputStreamAt(0) };
            DataWriter dw{ os };

            const auto arrBufSize = 8 * 1024;
            std::array<uint8_t, arrBufSize> arrBuf;

            mz_zip_reader_extract_iter_state* pState = mz_zip_reader_extract_iter_new(&zip_archive, i, 0);
            //size_t bytesRead{ 0 };
            while (size_t bytesRead{ mz_zip_reader_extract_iter_read(pState, static_cast<void*>(arrBuf.data()), arrBuf.size()) })
            {
                array_view<const uint8_t> view{ arrBuf.data(), arrBuf.data() + bytesRead };
                dw.WriteBytes(view);
            }
            status = mz_zip_reader_extract_iter_free(pState);
            assert(status);

            co_await dw.StoreAsync();
            co_await dw.FlushAsync();

            dw.Close();
            os.Close();
            stream.Close();
        }
    }

    status = mz_zip_reader_end(&zip_archive);
    assert(status);
}

/*
 * This is native-side of the RemotePackage.download method
 */
fire_and_forget CodePush::CodePush::DownloadUpdateAsync(JsonObject updatePackage, bool notifyProgress, ReactPromise<JsonObject> promise) noexcept 
{
    auto mutableUpdatePackage{ updatePackage };

    auto storageFolder{ Windows::Storage::ApplicationData::Current().LocalFolder() };

    wstring_view newUpdateHash{ updatePackage.GetNamedString(L"packageHash") };
    StorageFolder newUpdateFolder{ nullptr };

    auto downloadUrl{ updatePackage.GetNamedString(L"downloadUrl") };
    const uint32_t BufferSize{ 8 * 1024 };

    HttpClient client;
    auto headers{ client.DefaultRequestHeaders() };
    headers.Append(L"Accept-Encoding", L"identity");

    auto downloadFile{ co_await storageFolder.CreateFileAsync(L"download.zip", Windows::Storage::CreationCollisionOption::ReplaceExisting) };

    HttpRequestMessage reqm{ HttpMethod::Get(), Uri(downloadUrl) };
    auto resm{ co_await client.SendRequestAsync(reqm, HttpCompletionOption::ResponseHeadersRead) };
    auto totalBytes{ resm.Content().Headers().ContentLength().GetInt64() };
    auto inputStream{ co_await resm.Content().ReadAsInputStreamAsync() };
    auto outputStream{ co_await downloadFile.OpenAsync(Windows::Storage::FileAccessMode::ReadWrite) };

    bool isZip{ true };

    UINT64 receivedBytes{ 0 };
    for (;;)
    {
        auto outputBuffer{ co_await inputStream.ReadAsync(Buffer{ BufferSize }, BufferSize, InputStreamOptions::None) };

        if (outputBuffer.Length() == 0)
        {
            break;
        }
        co_await outputStream.WriteAsync(outputBuffer);

        receivedBytes += outputBuffer.Length();

        if (notifyProgress)
        {
            m_context.CallJSFunction(
                L"RCTDeviceEventEmitter",
                L"emit",
                L"CodePushDownloadProgress",
                JSValueObject{
                    {"totalBytes", totalBytes },
                    {"receivedBytes", receivedBytes } });
        }
    }

    inputStream.Close();
    outputStream.Close();

    if (isZip)
    {
        auto unzippedFolderName{ L"unzipped" };
        auto unzippedFolder = co_await storageFolder.CreateFolderAsync(unzippedFolderName, CreationCollisionOption::ReplaceExisting);
        co_await UnzipAsync(downloadFile, unzippedFolder);
        downloadFile.DeleteAsync();

        auto relativeBundlePath1{ co_await FindFilePathAsync(unzippedFolder, L"index.windows.bundle") };
        auto relativeBundlePath{ path(newUpdateHash) / std::wstring_view(relativeBundlePath1) };

        co_await unzippedFolder.RenameAsync(newUpdateHash, NameCollisionOption::ReplaceExisting);
        newUpdateFolder = unzippedFolder;

        //mutableUpdatePackage["bundlePath"] = to_string(relativeBundlePath.wstring());
        mutableUpdatePackage.Insert(L"bundlePath", JsonValue::CreateStringValue(relativeBundlePath.wstring()));
        promise.Resolve(mutableUpdatePackage);
    }
    else
    {
        // Rename the file
        co_await downloadFile.RenameAsync(L"index.windows.bundle", Windows::Storage::NameCollisionOption::ReplaceExisting);
    }

    auto newUpdateMetadataPath{ path(newUpdateHash) / "CodePush" / "assets" / "app.json" };
    auto newUpdateMetadataFile{ co_await CreateFileFromPathAsync(storageFolder, newUpdateMetadataPath) };

    co_await FileIO::WriteTextAsync(newUpdateMetadataFile, mutableUpdatePackage.Stringify());

    co_return;

    /*
    auto mutableUpdatePackage{ updatePackage };
    path binaryBundlePath{ GetBinaryBundlePath() };
    if (!binaryBundlePath.empty())
    {
        mutableUpdatePackage.Insert(BinaryBundleDateKey, JsonValue::CreateStringValue(CodePushUpdateUtils::ModifiedDateStringOfFileAtPath(binaryBundlePath)));
    }

    co_return;
    */
}

/*
RCT_EXPORT_METHOD(downloadUpdate:(NSDictionary*)updatePackage
                  notifyProgress:(BOOL)notifyProgress
                        resolver:(RCTPromiseResolveBlock)resolve
                        rejecter:(RCTPromiseRejectBlock)reject)
{
    NSDictionary *mutableUpdatePackage = [updatePackage mutableCopy];
    NSURL *binaryBundleURL = [CodePush binaryBundleURL];
    if (binaryBundleURL != nil) {
        [mutableUpdatePackage setValue:[CodePushUpdateUtils modifiedDateStringOfFileAtURL:binaryBundleURL]
                                forKey:BinaryBundleDateKey];
    }

    if (notifyProgress) {
        // Set up and unpause the frame observer so that it can emit
        // progress events every frame if the progress is updated.
        _didUpdateProgress = NO;
        self.paused = NO;
    }

    NSString * publicKey = [[CodePushConfig current] publicKey];

    [CodePushPackage
        downloadPackage:mutableUpdatePackage
        expectedBundleFileName:[bundleResourceName stringByAppendingPathExtension:bundleResourceExtension]
        publicKey:publicKey
        operationQueue:_methodQueue
        // The download is progressing forward
        progressCallback:^(long long expectedContentLength, long long receivedContentLength) {
            // Update the download progress so that the frame observer can notify the JS side
            _latestExpectedContentLength = expectedContentLength;
            _latestReceivedConentLength = receivedContentLength;
            _didUpdateProgress = YES;

            // If the download is completed, stop observing frame
            // updates and synchronously send the last event.
            if (expectedContentLength == receivedContentLength) {
                _didUpdateProgress = NO;
                self.paused = YES;
                [self dispatchDownloadProgressEvent];
            }
        }
        // The download completed
        doneCallback:^{
            NSError *err;
            NSDictionary *newPackage = [CodePushPackage getPackage:mutableUpdatePackage[PackageHashKey] error:&err];

            if (err) {
                return reject([NSString stringWithFormat: @"%lu", (long)err.code], err.localizedDescription, err);
            }
            resolve(newPackage);
        }
        // The download failed
        failCallback:^(NSError *err) {
            if ([CodePushErrorUtils isCodePushError:err]) {
                [self saveFailedUpdate:mutableUpdatePackage];
            }

            // Stop observing frame updates if the download fails.
            _didUpdateProgress = NO;
            self.paused = YES;
            reject([NSString stringWithFormat: @"%lu", (long)err.code], err.localizedDescription, err);
        }];
}
*/

/*
 * This is the native side of the CodePush.getConfiguration method. It isn't
 * currently exposed via the "react-native-code-push" module, and is used
 * internally only by the CodePush.checkForUpdate method in order to get the
 * app version, as well as the deployment key that was configured in the Info.plist file.
 */
void CodePush::CodePush::GetConfiguration(ReactPromise<IJsonValue> promise) noexcept 
{
    JsonObject configMap;
    configMap.Insert(L"appVersion", JsonValue::CreateStringValue(L"1.0.0"));
    configMap.Insert(L"deploymentKey", JsonValue::CreateStringValue(L"BJwawsbtm8a1lTuuyN0GPPXMXCO1oUFtA_jJS"));
    configMap.Insert(L"serverUrl", JsonValue::CreateStringValue(L"https://codepush.appcenter.ms/"));
    promise.Resolve(configMap);
}

/*
RCT_EXPORT_METHOD(getConfiguration:(RCTPromiseResolveBlock)resolve
                          rejecter:(RCTPromiseRejectBlock)reject)
{
    NSDictionary *configuration = [[CodePushConfig current] configuration];
    NSError *error;
    if (isRunningBinaryVersion) {
        // isRunningBinaryVersion will not get set to "YES" if running against the packager.
        NSString *binaryHash = [CodePushUpdateUtils getHashForBinaryContents:[CodePush binaryBundleURL] error:&error];
        if (error) {
            CPLog(@"Error obtaining hash for binary contents: %@", error);
            resolve(configuration);
            return;
        }

        if (binaryHash == nil) {
            // The hash was not generated either due to a previous unknown error or the fact that
            // the React Native assets were not bundled in the binary (e.g. during dev/simulator)
            // builds.
            resolve(configuration);
            return;
        }

        NSMutableDictionary *mutableConfiguration = [configuration mutableCopy];
        [mutableConfiguration setObject:binaryHash forKey:PackageHashKey];
        resolve(mutableConfiguration);
        return;
    }

    resolve(configuration);
}
*/

/*
 * This method is the native side of the CodePush.getUpdateMetadata method.
 */
fire_and_forget CodePush::CodePush::GetUpdateMetadataAsync(CodePushUpdateState updateState, ReactPromise<JsonObject> promise) noexcept { co_return; }

/*
 * This method is the native side of the LocalPackage.install method.
 */
fire_and_forget CodePush::CodePush::InstallUpdateAsync(JsonObject updatePackage, CodePushInstallMode installMode, int minimumBackgroundDuration, ReactPromise<void> promise) noexcept { co_return; }

/*
 * This method isn't publicly exposed via the "react-native-code-push"
 * module, and is only used internally to populate the RemotePackage.failedInstall property.
 */
void CodePush::CodePush::IsFailedUpdate(wstring packageHash, ReactPromise<bool> promise) noexcept {}

void CodePush::CodePush::SetLatestRollbackInfo(wstring packageHash) noexcept {}

void CodePush::CodePush::GetLatestRollbackInfo(ReactPromise<JsonObject> promise) noexcept {}

/*
 * This method isn't publicly exposed via the "react-native-code-push"
 * module, and is only used internally to populate the LocalPackage.isFirstRun property.
 */
void CodePush::CodePush::IsFirstRun(wstring packageHash, ReactPromise<bool> promise) noexcept {}

/*
 * This method is the native side of the CodePush.notifyApplicationReady() method.
 */
void CodePush::CodePush::NotifyApplicationReady(ReactPromise<IJsonValue> promise) noexcept
{
    RemovePendingUpdate();
    promise.Resolve(JsonValue::CreateNullValue());
}

/*
RCT_EXPORT_METHOD(notifyApplicationReady:(RCTPromiseResolveBlock)resolve
                                rejecter:(RCTPromiseRejectBlock)reject)
{
    [CodePush removePendingUpdate];
    resolve(nil);
}
*/

void CodePush::CodePush::Allow(ReactPromise<JSValue> promise) noexcept 
{
    CodePushUtils::Log(L"Re-allowing restarts.");
    m_allowed = true;

    if (m_restartQueue.size() > 0)
    {
        CodePushUtils::Log(L"Executing pending restart.");
        auto buf{ m_restartQueue[0] };
        m_restartQueue.erase(m_restartQueue.begin());
        RestartAppInternal(buf);
    }

    promise.Resolve(JSValue::Null);
}

void CodePush::CodePush::ClearPendingRestart() noexcept {}

void CodePush::CodePush::Disallow(ReactPromise<JSValue> promise) noexcept
{
    CodePushUtils::Log(L"Disallowing restarts.");
    m_allowed = false;
    promise.Resolve(JSValue::Null);
}

/*
 * This method is the native side of the CodePush.restartApp() method.
 */
void CodePush::CodePush::RestartApp(bool onlyIfUpdateIsPending, ReactPromise<JSValue> promise) noexcept 
{
    RestartAppInternal(onlyIfUpdateIsPending);
    promise.Resolve(JSValue::Null);
}

/*
 * This method clears CodePush's downloaded updates.
 * It is needed to switch to a different deployment if the current deployment is more recent.
 * Note: we don’t recommend to use this method in scenarios other than that (CodePush will call this method
 * automatically when needed in other cases) as it could lead to unpredictable behavior.
 */
void CodePush::CodePush::ClearUpdates() noexcept {}

// #pragma mark - JavaScript-exported module methods (Private)

/*
 * This method is the native side of the CodePush.downloadAndReplaceCurrentBundle()
 * method, which replaces the current bundle with the one downloaded from
 * removeBundleUrl. It is only to be used during tests and no-ops if the test
 * configuration flag is not set.
 */
void CodePush::CodePush::DownloadAndReplaceCurrentBundle(wstring remoteBundleUrl) noexcept {}

/*
 * This method is checks if a new status update exists (new version was installed,
 * or an update failed) and return its details (version label, status).
 */
fire_and_forget CodePush::CodePush::GetNewStatusReportAsync(ReactPromise<JsonObject> promise) noexcept 
{
    /*
    if (needToReportRollback)
    {
        needToReportRollback = false;
        // save stuff to LocalSettings
    }
    else if (m_isFirstRunAfterUpdate)
    {
        auto currentPackage = co_await CodePushPackage::GetCurrentPackageAsync();
        promise.Resolve(CodePushTelemetryManager::GetUpdateReport(currentPackage));
        co_return;
    }
    else if (isRunningBinaryVersion)
    {
        auto appVersion{ CodePushConfig::Current().GetAppVersion() };
        promise.Resolve(CodePushTelemetryManager::GetBinaryUpdateReport(appVersion));
        co_return;
    }
    else
    {
        auto retryStatusReport{ CodePushTelemetryManager::GetRetryStatusReport() };
        if (retryStatusReport != nullptr)
        {
            promise.Resolve(retryStatusReport);
            co_return;
        }
    }
    */

    promise.Resolve(JsonObject{});
    co_return;
}

/*
RCT_EXPORT_METHOD(getNewStatusReport:(RCTPromiseResolveBlock)resolve
                            rejecter:(RCTPromiseRejectBlock)reject)
{
    if (needToReportRollback) {
        needToReportRollback = NO;
        NSUserDefaults *preferences = [NSUserDefaults standardUserDefaults];
        NSMutableArray *failedUpdates = [preferences objectForKey:FailedUpdatesKey];
        if (failedUpdates) {
            NSDictionary *lastFailedPackage = [failedUpdates lastObject];
            if (lastFailedPackage) {
                resolve([CodePushTelemetryManager getRollbackReport:lastFailedPackage]);
                return;
            }
        }
    } else if (_isFirstRunAfterUpdate) {
        NSError *error;
        NSDictionary *currentPackage = [CodePushPackage getCurrentPackage:&error];
        if (!error && currentPackage) {
            resolve([CodePushTelemetryManager getUpdateReport:currentPackage]);
            return;
        }
    } else if (isRunningBinaryVersion) {
        NSString *appVersion = [[CodePushConfig current] appVersion];
        resolve([CodePushTelemetryManager getBinaryUpdateReport:appVersion]);
        return;
    } else {
        NSDictionary *retryStatusReport = [CodePushTelemetryManager getRetryStatusReport];
        if (retryStatusReport) {
            resolve(retryStatusReport);
            return;
        }
    }

    resolve(nil);
}
*/

void CodePush::CodePush::RecordStatusReported(JsonObject statusReport) noexcept {}

void CodePush::CodePush::SaveStatusReportForRetry(JsonObject statusReport) noexcept {}

// Helper functions for reading and sending JsonValues to and from JavaScript
namespace winrt::Microsoft::ReactNative
{
    using namespace winrt::Windows::Data::Json;

    void WriteValue(IJSValueWriter const& writer, IJsonValue const& value) noexcept
    {
        switch (value.ValueType())
        {
        case JsonValueType::Object:
            writer.WriteObjectBegin();
            for (const auto& pair : value.GetObject())
            {
                writer.WritePropertyName(pair.Key());
                WriteValue(writer, pair.Value());
            }
            writer.WriteObjectEnd();
            break;
        case JsonValueType::Array:
            writer.WriteArrayBegin();
            for (const auto& elem : value.GetArray())
            {
                WriteValue(writer, elem);
            }
            writer.WriteArrayEnd();
            break;
        case JsonValueType::Boolean:
            writer.WriteBoolean(value.GetBoolean());
            break;
        case JsonValueType::Number:
            writer.WriteDouble(value.GetNumber());
            break;
        case JsonValueType::String:
            writer.WriteString(value.GetString());
            break;
        case JsonValueType::Null:
            writer.WriteNull();
            break;
        }
    }

    void ReadValue(IJSValueReader const& reader, /*out*/ JsonObject& value) noexcept
    {
        if (reader.ValueType() == JSValueType::Object)
        {
            hstring propertyName;
            while (reader.GetNextObjectProperty(propertyName))
            {
                value.Insert(propertyName, ReadValue<IJsonValue>(reader));
            }
        }
    }

    void ReadValue(IJSValueReader const& reader, /*out*/ IJsonValue& value) noexcept
    {
        JsonObject valueObject;
        JsonArray valueArray;
        hstring propertyName;
        switch (reader.ValueType())
        {
        case JSValueType::Object:
            while (reader.GetNextObjectProperty(propertyName))
            {
                valueObject.Insert(propertyName, ReadValue<IJsonValue>(reader));
            }
            value = valueObject;
            break;
        case JSValueType::Array:
            while (reader.GetNextArrayItem())
            {
                valueArray.Append(ReadValue<IJsonValue>(reader));
            }
            value = valueArray;
            break;
        case JSValueType::Boolean:
            value = JsonValue::CreateBooleanValue(reader.GetBoolean());
            break;
        case JSValueType::Double:
            value = JsonValue::CreateNumberValue(reader.GetDouble());
            break;
        case JSValueType::Int64:
            value = JsonValue::CreateNumberValue(static_cast<double>(reader.GetInt64()));
            break;
        case JSValueType::String:
            value = JsonValue::CreateStringValue(reader.GetString());
            break;
        case JSValueType::Null:
            value = JsonValue::CreateNullValue();
            break;
        }
    }
}