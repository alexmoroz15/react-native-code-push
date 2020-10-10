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

IAsyncOperation<StorageFile> CodePush::CodePush::GetBinaryAsync() 
{ 
    //return nullptr;
    Uri binaryUri{ L"ms-appx://" };
    auto binaryFolder{ co_await StorageFolder::GetFolderFromPathAsync(binaryUri.Path()) };
    auto binaryFolderFiles{ co_await binaryFolder.GetFilesAsync() };
    for (const auto& file : binaryFolderFiles)
    {
        wstring fileName{ file.Name() };
        if (fileName.rfind(L".exe") != wstring::npos)
        {
            co_return file;
        }
    }
    co_return nullptr;
}

IAsyncOperation<StorageFile> CodePush::CodePush::GetBundleFileAsync() { co_return nullptr; }
//path CodePush::CodePush::GetBundlePath() { return nullptr; }

void CodePush::CodePush::OverrideAppVersion(wstring appVersion) {}
void CodePush::CodePush::SetDeploymentKey(wstring deploymentKey) {}

bool CodePush::CodePush::IsFailedHash(wstring_view packageHash) { return false; }

JsonObject CodePush::CodePush::GetRollbackInfo() { return nullptr; }
//void SetLatestRollbackInfo(wstring packageHash);
int CodePush::CodePush::GetRollbackCountForPackage(wstring packageHash, JsonObject latestRollbackInfo) { return 0; }

bool CodePush::CodePush::IsPendingUpdate(wstring packageHash) { return false; }

bool CodePush::CodePush::IsUsingTestConfiguration() { return false; }
void CodePush::CodePush::SetUsingTestConfiguration() {}
//void ClearUpdates();

void CodePush::CodePush::DispatchDownloadProgressEvent() 
{
    // Notify the script-side about the progress
    m_context.CallJSFunction(
        L"RCTDeviceEventEmitter",
        L"emit",
        L"CodePushDownloadProgress",
        JSValueObject{
            {"totalBytes", m_latestExpectedContentLength },
            {"receivedBytes", m_latestReceivedContentLength } });
}

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

/*
 * When an update failed to apply, this method can be called
 * to store its hash so that it can be ignored on future
 * attempts to check the server for an update.
 */
void CodePush::CodePush::SaveFailedUpdate(JsonObject& failedPackage)
{
    if (IsFailedHash(failedPackage.GetNamedString(PackageHashKey)))
    {
        return;
    }

    auto localSettings{ ApplicationData::Current().LocalSettings() };
    auto failedUpdates{ localSettings.Values().TryLookup(FailedUpdatesKey).try_as<JsonArray>() };
    if (failedUpdates == nullptr)
    {
        failedUpdates = JsonArray{};
    }

    failedUpdates.Append(failedPackage);
    localSettings.Values().Insert(FailedUpdatesKey, failedUpdates);
}

void CodePush::CodePush::Initialize(ReactContext const& reactContext) noexcept
{
    m_context = reactContext;

    auto res = reactContext.Properties().Handle().Get(ReactPropertyBagHelper::GetName(nullptr, L"MyReactNativeHost"));
    m_host = res.as<ReactNativeHost>();

    m_codePushConfig = CodePushConfig::Init(reactContext);
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
    auto binary{ co_await GetBinaryAsync() };
    /*
    path binaryBundlePath{ GetBinaryBundlePath() };
    if (!binaryBundlePath.empty())
    {
        mutableUpdatePackage.Insert(BinaryBundleDateKey, JsonValue::CreateStringValue(CodePushUpdateUtils::ModifiedDateStringOfFileAtPath(binaryBundlePath)));
    }
    */

    bool paused{ false };
    if (notifyProgress)
    {
        m_didUpdateProgress = false;
        paused = false;
    }

    //auto publicKey{ CodePushConfig::Current().GetPublicKey() };
    auto publicKey{ m_codePushConfig.GetPublicKey() };

    co_await CodePushPackage::DownloadPackageAsync(
        mutableUpdatePackage, 
        (co_await GetBundleFileAsync()).Path(), 
        publicKey,
        /* progressCallback */ [=](int64_t expectedContentLength, int64_t receivedContentLength) {
            // Update the download progress so that the frame observer can notify the JS side
            m_latestExpectedContentLength = expectedContentLength;
            m_latestReceivedContentLength = receivedContentLength;
            m_didUpdateProgress = true;

            // If the download is completed, stop observing frame
            // updates and synchronously send the last event.
            if (expectedContentLength == receivedContentLength) {
                m_didUpdateProgress = false;
                //paused = true;
                DispatchDownloadProgressEvent();
            }
        },
        /* doneCallback */ [=] {
            auto newPackage{ CodePushPackage::GetPackageAsync(mutableUpdatePackage.GetNamedString(PackageHashKey)).GetResults() };
            promise.Resolve(newPackage);
        },
        /* failCallback */ [=](const hresult_error& ex) {
            // If the error is a codepush error...
            auto updatePackage = mutableUpdatePackage;
            SaveFailedUpdate(updatePackage);
            
            // Stop observing frame updates if the download fails.
            m_didUpdateProgress = false;
            //paused = true;
            promise.Reject(ex.message().c_str());
        });

    co_return;
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
    auto configuration{ m_codePushConfig.GetConfiguration() };
    if (isRunningBinaryVersion)
    {
        // ...
    }
    promise.Resolve(configuration);
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
fire_and_forget CodePush::CodePush::GetUpdateMetadataAsync(CodePushUpdateState updateState, ReactPromise<JsonObject> promise) noexcept 
{
    /*
    // Get the current package
    auto currentPackage{ co_await CodePushPackage::GetCurrentPackageAsync() };
    
    //auto currentUpdateIsPending{ currentPackage.GetNamedBoolean(L"isPending", false) };
    auto currentUpdateIsPending = false;

    if (updateState == CodePushUpdateState::PENDING && !currentUpdateIsPending)
    {
        promise.Resolve(JsonValue::CreateNullValue().try_as<IJsonValue>());
    }
    else if (updateState == CodePushUpdateState::RUNNING && currentUpdateIsPending)
    {
        auto previousPackage{ co_await CodePushPackage::GetPreviousPackageAsync() };
        if (previousPackage == nullptr)
        {
            promise.Resolve(JsonValue::CreateNullValue().try_as<IJsonValue>());
            co_return;
        }

        promise.Resolve(previousPackage);
    }
    else
    {
        if (IsRunningBinaryVersion())
        {
            currentPackage.Insert(L"_isDebugOnly", JsonValue::CreateBooleanValue(true));
        }

        currentPackage.Insert(L"isPending", JsonValue::CreateBooleanValue(currentUpdateIsPending));
        promise.Resolve(currentPackage);
    }
    co_return;
    */
    co_return; 
}

/*
RCT_EXPORT_METHOD(getUpdateMetadata:(CodePushUpdateState)updateState
                           resolver:(RCTPromiseResolveBlock)resolve
                           rejecter:(RCTPromiseRejectBlock)reject)
{
    NSError *error;
    NSMutableDictionary *package = [[CodePushPackage getCurrentPackage:&error] mutableCopy];

    if (error) {
        return reject([NSString stringWithFormat: @"%lu", (long)error.code], error.localizedDescription, error);
    } else if (package == nil) {
        // The app hasn't downloaded any CodePush updates yet,
        // so we simply return nil regardless if the user
        // wanted to retrieve the pending or running update.
        return resolve(nil);
    }

    // We have a CodePush update, so let's see if it's currently in a pending state.
    BOOL currentUpdateIsPending = [[self class] isPendingUpdate:[package objectForKey:PackageHashKey]];

    if (updateState == CodePushUpdateStatePending && !currentUpdateIsPending) {
        // The caller wanted a pending update
        // but there isn't currently one.
        resolve(nil);
    } else if (updateState == CodePushUpdateStateRunning && currentUpdateIsPending) {
        // The caller wants the running update, but the current
        // one is pending, so we need to grab the previous.
        resolve([CodePushPackage getPreviousPackage:&error]);
    } else {
        // The current package satisfies the request:
        // 1) Caller wanted a pending, and there is a pending update
        // 2) Caller wanted the running update, and there isn't a pending
        // 3) Caller wants the latest update, regardless if it's pending or not
        if (isRunningBinaryVersion) {
            // This only matters in Debug builds. Since we do not clear "outdated" updates,
            // we need to indicate to the JS side that somehow we have a current update on
            // disk that is not actually running.
            [package setObject:@(YES) forKey:@"_isDebugOnly"];
        }

        // Enable differentiating pending vs. non-pending updates
        [package setObject:@(currentUpdateIsPending) forKey:PackageIsPendingKey];
        resolve(package);
    }
}
*/

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
 * Note: we don�t recommend to use this method in scenarios other than that (CodePush will call this method
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
fire_and_forget CodePush::CodePush::GetNewStatusReportAsync(ReactPromise<IJsonValue> promise) noexcept 
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

    promise.Resolve(JsonValue::CreateNullValue());
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
        if (reader.ValueType() == JSValueType::Object)
        {
            JsonObject valueObject;
            hstring propertyName;
            while (reader.GetNextObjectProperty(propertyName))
            {
                valueObject.Insert(propertyName, ReadValue<IJsonValue>(reader));
            }
            value = valueObject;
        }
        else if (reader.ValueType() == JSValueType::Array)
        {
            JsonArray valueArray;
            while (reader.GetNextArrayItem())
            {
                valueArray.Append(ReadValue<IJsonValue>(reader));
            }
            value = valueArray;
        }
        else
        {
            switch (reader.ValueType())
            {
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
}