#include "pch.h"

#include "CodePushNativeModule.h"
#include "CodePushUtils.h"
#include "CodePushUpdateUtils.h"
#include "CodePushPackage.h"
#include "CodePushTelemetryManager.h"
#include "CodePushConfig.h"
#include "App.h"
//#include "JSValueAdditions.h"

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
#include "winrt/Windows.ApplicationModel.h"
#include "winrt/Windows.Storage.FileProperties.h"

using namespace winrt;
using namespace Microsoft::ReactNative;
using namespace Windows::Data::Json;
using namespace Windows::Web::Http;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::Foundation;

using namespace std;
using namespace filesystem;

using namespace CodePush;

IAsyncOperation<StorageFile> CodePushNativeModule::GetBinaryBundleAsync() 
{
    auto appXFolder{ Windows::ApplicationModel::Package::Current().InstalledLocation() };
    auto bundleFolder{ (co_await appXFolder.TryGetItemAsync(L"Bundle")).try_as<StorageFolder>() };
    if (bundleFolder == nullptr)
    {
        co_return nullptr;
    }
    auto bundleFile{ (co_await bundleFolder.TryGetItemAsync(L"index.windows.bundle")).try_as<StorageFile>() };
    co_return bundleFile;
}


IAsyncOperation<StorageFile> CodePushNativeModule::GetBundleFileAsync()
{ 
    co_return nullptr; 
}

// Rather than store files in the library files, CodePush for ReactNativeWindows will use AppData folders.
StorageFolder CodePushNativeModule::GetLocalStorageFolder()
{
    return ApplicationData::Current().LocalFolder();
}

path CodePushNativeModule::GetLocalStoragePath()
{
    return wstring_view(GetLocalStorageFolder().Path());
}

void CodePushNativeModule::OverrideAppVersion(wstring_view appVersion) 
{
    m_codePushConfig.SetAppVersion(appVersion);
}

void CodePushNativeModule::SetDeploymentKey(wstring_view deploymentKey) 
{
    m_codePushConfig.SetDeploymentKey(deploymentKey);
}

/*
 * This method checks to see whether a specific package hash
 * has previously failed installation.
 */
bool CodePushNativeModule::IsFailedHash(wstring_view packageHash) 
{ 
    auto localSettings{ ApplicationData::Current().LocalSettings() };
    auto failedUpdatesData{ localSettings.Values().TryLookup(FailedUpdatesKey) };
    if (failedUpdatesData == nullptr)
    {
        return false;
    }
    auto failedUpdatesString{ unbox_value<hstring>(failedUpdatesData) };
    JsonArray failedUpdates;
    auto success{ JsonArray::TryParse(failedUpdatesString, failedUpdates) };
    if (!success || packageHash.empty())
    {
        return false;
    }
    else
    {
        for (const auto& failedPackage : failedUpdates)
        {
            // We don't have to worry about backwards compatability, but just to be safe...
            if (failedPackage.ValueType() == JsonValueType::Object)
            {
                auto failedPackageHash{ failedPackage.GetObject().GetNamedString(PackageHashKey) };
                if (packageHash == failedPackageHash)
                {
                    return true;
                }
            }
        }

        return false;
    }
}

/*
 * This method is used to get information about the latest rollback.
 * This information will be used to decide whether the application
 * should ignore the update or not.
 */
JsonObject CodePushNativeModule::GetRollbackInfo() { return nullptr; }

/*
 * This method is used to get the count of rollback for the package
 * using the latest rollback information.
 */
int CodePushNativeModule::GetRollbackCountForPackage(wstring_view packageHash, const JsonObject& latestRollbackInfo) 
{
    auto oldPackageHash{ latestRollbackInfo.GetNamedString(LatestRollbackPackageHashKey, L"null") };
    if (packageHash == oldPackageHash)
    {
        auto oldCount{ latestRollbackInfo.GetNamedNumber(LatestRollbackCountKey, 0) };
        return oldCount;
    }
    return 0; 
}

/*
 * This method checks to see whether a specific package hash
 * represents a downloaded and installed update, that hasn't
 * been applied yet via an app restart.
 */
bool CodePushNativeModule::IsPendingUpdate(wstring_view packageHash) 
{ 
    auto localSettings{ ApplicationData::Current().LocalSettings() };
    auto pendingUpdateData{ localSettings.Values().TryLookup(PendingUpdateKey) };
    if (pendingUpdateData != nullptr)
    {
        auto pendingUpdateString{ unbox_value<hstring>(pendingUpdateData) };
        JsonObject pendingUpdate;
        auto success{ JsonObject::TryParse(pendingUpdateString, pendingUpdate) };
        
        // If there is a pending update whose "state" isn't loading, then we consider it "pending".
        // Additionally, if a specific hash was provided, we ensure it matches that of the pending update.
        auto updateIsPending{ success &&
            pendingUpdate != nullptr &&
            pendingUpdate.GetNamedBoolean(PendingUpdateIsLoadingKey, false) == false &&
            (packageHash.empty() || pendingUpdate.GetNamedString(PendingUpdateHashKey, L"null") == packageHash) };

        return updateIsPending;
    }
    return false;
}

/*
 * This returns a boolean value indicating whether CodePush has
 * been set to run under a test configuration.
 */
bool CodePushNativeModule::IsUsingTestConfiguration() 
{ 
    return testConfigurationFlag;
}

/*
 * This is used to enable an environment in which tests can be run.
 * Specifically, it flips a boolean flag that causes bundles to be
 * saved to a test folder and enables the ability to modify
 * installed bundles on the fly from JavaScript.
 */
void CodePushNativeModule::SetUsingTestConfiguration(bool shouldUseTestConfiguration) 
{
    testConfigurationFlag = shouldUseTestConfiguration;
}

void CodePushNativeModule::DispatchDownloadProgressEvent()
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

void CodePushNativeModule::LoadBundle()
{
    m_host.ReloadInstance();
}

/*
 * This method is used to clear away failed updates in the event that
 * a new app store binary is installed.
 */

void CodePushNativeModule::RemoveFailedUpdates()
{
    auto localSettings{ ApplicationData::Current().LocalSettings() };
    localSettings.Values().TryRemove(FailedUpdatesKey);
}

/*
 * This method is used to register the fact that a pending
 * update succeeded and therefore can be removed.
 */
void CodePushNativeModule::RemovePendingUpdate()
{
    // remove pending update from LocalSettings
    auto localSettings{ ApplicationData::Current().LocalSettings() };
    localSettings.Values().TryRemove(PendingUpdateKey);
}

void CodePushNativeModule::RestartAppInternal(bool onlyIfUpdateIsPending)
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
    if (!onlyIfUpdateIsPending || IsPendingUpdate(L""))
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
void CodePushNativeModule::SaveFailedUpdate(JsonObject& failedPackage)
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
    localSettings.Values().Insert(FailedUpdatesKey, box_value(failedUpdates.Stringify()));
}

/*
 * When an update is installed whose mode isn't IMMEDIATE, this method
 * can be called to store the pending update's metadata (e.g. packageHash)
 * so that it can be used when the actual update application occurs at a later point.
 */
void CodePushNativeModule::SavePendingUpdate(wstring_view packageHash, bool isLoading)
{
    // Since we're not restarting, we need to store the fact that the update
    // was installed, but hasn't yet become "active".
    auto localSettings{ ApplicationData::Current().LocalSettings() };
    JsonObject pendingUpdate{};
    pendingUpdate.Insert(PendingUpdateHashKey, JsonValue::CreateStringValue(packageHash));
    pendingUpdate.Insert(PendingUpdateIsLoadingKey, JsonValue::CreateBooleanValue(isLoading));
    localSettings.Values().Insert(PendingUpdateKey, box_value(pendingUpdate.Stringify()));
}

void CodePushNativeModule::Initialize(ReactContext const& reactContext) noexcept
{
    m_context = reactContext;

    auto res = reactContext.Properties().Handle().Get(ReactPropertyBagHelper::GetName(nullptr, L"MyReactNativeHost"));
    m_host = res.as<ReactNativeHost>();

    m_codePushConfig = CodePushConfig::Init(reactContext);
}

void CodePushNativeModule::GetConstants(winrt::Microsoft::ReactNative::ReactConstantProvider& constants) noexcept
{
    constants.Add(L"codePushInstallModeImmediate", CodePushInstallMode::IMMEDIATE);
    constants.Add(L"codePushInstallModeOnNextRestart", CodePushInstallMode::ON_NEXT_RESTART);
    constants.Add(L"codePushInstallModeOnNextResume", CodePushInstallMode::ON_NEXT_RESUME);
    constants.Add(L"codePushInstallModeOnNextSuspend", CodePushInstallMode::ON_NEXT_SUSPEND);

    constants.Add(L"codePushUpdateStateRunning", CodePushUpdateState::RUNNING);
    constants.Add(L"codePushUpdateStatePending", CodePushUpdateState::PENDING);
    constants.Add(L"codePushUpdateStateLatest", CodePushUpdateState::LATEST);
}

/*
 * This is native-side of the RemotePackage.download method
 */
fire_and_forget CodePushNativeModule::DownloadUpdateAsync(JsonObject updatePackage, bool notifyProgress, ReactPromise<IJsonValue> promise) noexcept
{
    //auto mutableUpdatePackage{ updatePackage };
    auto binaryBundle{ co_await GetBinaryBundleAsync() };
    if (binaryBundle != nullptr)
    {
        auto basicProperties{ co_await binaryBundle.GetBasicPropertiesAsync() };
        auto modifiedDate{ basicProperties.DateModified() };
        //auto mtime{ modifiedDate.time_since_epoch() / std::chrono::seconds(1) - UNIX_EPOCH_IN_WINRT_SECONDS };
        auto mtime{ clock::to_time_t(modifiedDate) };
        updatePackage.Insert(BinaryBundleDateKey, JsonValue::CreateStringValue(to_hstring(mtime)));
    }

    auto publicKey{ m_codePushConfig.GetPublicKey() };

    try
    {
        co_await CodePushPackage::DownloadPackageAsync(
            updatePackage,
            /* m_host.InstanceSettings().JavaScriptBundleFile() */ L"index.windows.bundle",
            /* publicKey */ ((publicKey.has_value()) ? publicKey.value() : L""),
            /* progressCallback */ [=](int64_t expectedContentLength, int64_t receivedContentLength) {
                // React-Native-Windows doesn't have a frame observer to my knowledge.
                if (notifyProgress)
                {
                    m_latestExpectedContentLength = expectedContentLength;
                    m_latestReceivedContentLength = receivedContentLength;
                    DispatchDownloadProgressEvent();
                }
            });
    }
    catch (const hresult_error& ex)
    {
        // If the error is a codepush error...
        //auto updatePackage = mutableUpdatePackage;
        SaveFailedUpdate(updatePackage);

        // Stop observing frame updates if the download fails.
        m_didUpdateProgress = false;
        promise.Reject(ex.message().c_str());
    }

    auto newPackage{ co_await CodePushPackage::GetPackageAsync(updatePackage.GetNamedString(PackageHashKey)) };
    if (newPackage == nullptr)
    {
        promise.Resolve(JsonValue::CreateNullValue());
    }
    else
    {
        promise.Resolve(newPackage);
    }

    co_return;
}

/*
 * This is the native side of the CodePush.getConfiguration method. It isn't
 * currently exposed via the "react-native-code-push" module, and is used
 * internally only by the CodePush.checkForUpdate method in order to get the
 * app version, as well as the deployment key that was configured in the Info.plist file.
 */
fire_and_forget CodePushNativeModule::GetConfiguration(ReactPromise<IJsonValue> promise) noexcept 
{
    auto configuration{ m_codePushConfig.GetConfiguration() };
    if (isRunningBinaryVersion)
    {
        // isRunningBinaryVersion will not get set to "true" if running against the packager.
        hstring binaryHash;
        try
        {
            binaryHash = CodePushUpdateUtils::GetHashForBinaryContents(co_await GetBinaryBundleAsync());
        }
        catch(...)
        {
            CodePushUtils::Log(L"Error obtaining hash for binary contents.");
            promise.Resolve(configuration);
            co_return;
        }

        if (binaryHash.empty())
        {
            // The hash was not generated either due to a previous unknown error or the fact that
            // the React Native assets were not bundled in the binary (e.g. during dev/simulator)
            // builds.
            promise.Resolve(configuration);
            co_return;
        }

        configuration.Insert(PackageHashKey, JsonValue::CreateStringValue(binaryHash));
        promise.Resolve(configuration);
        co_return;
    }
    promise.Resolve(configuration);
}

/*
 * This method is the native side of the CodePush.getUpdateMetadata method.
 */
fire_and_forget CodePushNativeModule::GetUpdateMetadataAsync(CodePushUpdateState updateState, ReactPromise<IJsonValue> promise) noexcept 
{
    auto package{ co_await CodePushPackage::GetCurrentPackageAsync() };
    if (package == nullptr)
    {
        // The app hasn't downloaded any CodePush updates yet,
        // so we simply return nil regardless if the user
        // wanted to retrieve the pending or running update.
        promise.Resolve(JsonValue::CreateNullValue());
        co_return;
    }

    // We have a CodePush update, so let's see if it's currently in a pending state.
    bool currentUpdateIsPending{ IsPendingUpdate(package.GetNamedString(PackageHashKey)) };

    if (updateState == CodePushUpdateState::PENDING && !currentUpdateIsPending) {
        // The caller wanted a pending update
        // but there isn't currently one.
        promise.Resolve(JsonValue::CreateNullValue());
    }
    else if (updateState == CodePushUpdateState::RUNNING && currentUpdateIsPending) {
        // The caller wants the running update, but the current
        // one is pending, so we need to grab the previous.
        promise.Resolve(co_await CodePushPackage::GetPreviousPackageAsync());
    }
    else {
        // The current package satisfies the request:
        // 1) Caller wanted a pending, and there is a pending update
        // 2) Caller wanted the running update, and there isn't a pending
        // 3) Caller wants the latest update, regardless if it's pending or not
        if (isRunningBinaryVersion) {
            // This only matters in Debug builds. Since we do not clear "outdated" updates,
            // we need to indicate to the JS side that somehow we have a current update on
            // disk that is not actually running.
            package.Insert(L"_isDebugOnly", JsonValue::CreateBooleanValue(true));
        }

        // Enable differentiating pending vs. non-pending updates
        package.Insert(PackageIsPendingKey, JsonValue::CreateBooleanValue(currentUpdateIsPending));
        promise.Resolve(package);
    }
    co_return; 
}

/*
 * This method is the native side of the LocalPackage.install method.
 */
fire_and_forget CodePushNativeModule::InstallUpdateAsync(JsonObject updatePackage, CodePushInstallMode installMode, int minimumBackgroundDuration, ReactPromise<void> promise) noexcept 
{ 
    try
    {
        co_await CodePushPackage::InstallPackageAsync(updatePackage, IsPendingUpdate(L""));
    }
    catch (const hresult_error& ex)
    {
        promise.Reject(ex.message().c_str());
        co_return;
    }
    SavePendingUpdate(updatePackage.GetNamedString(PackageHashKey), false);
    m_installMode = installMode;
    if (m_installMode == CodePushInstallMode::ON_NEXT_RESUME || m_installMode == CodePushInstallMode::ON_NEXT_SUSPEND) {
        m_minimumBackgroundDuration = minimumBackgroundDuration;

        if (!m_hasResumeListener) {
            // Ensure we do not add the listener twice.
            // Register for app resume notifications so that we
            // can check for pending updates which support "restart on resume"
            
            // Need to find a way to do this.
            /*
            [[NSNotificationCenter defaultCenter]addObserver:self
                selector : @selector(applicationWillEnterForeground)
                name:UIApplicationWillEnterForegroundNotification
                object : RCTSharedApplication()];

            [[NSNotificationCenter defaultCenter]addObserver:self
                selector : @selector(applicationWillResignActive)
                name:UIApplicationWillResignActiveNotification
                object : RCTSharedApplication()];
            */

            m_hasResumeListener = true;
        }
    }

    // Signal to JS that the update has been applied.
    promise.Resolve();
    co_return; 
}

/*
 * This method isn't publicly exposed via the "react-native-code-push"
 * module, and is only used internally to populate the RemotePackage.failedInstall property.
 */
void CodePushNativeModule::IsFailedUpdate(wstring packageHash, ReactPromise<bool> promise) noexcept 
{
    auto isFailedHash{ IsFailedHash(packageHash) };
    promise.Resolve(isFailedHash);
}

/*
 * This method is used to save information about the latest rollback.
 * This information will be used to decide whether the application
 * should ignore the update or not.
 */
void CodePushNativeModule::SetLatestRollbackInfo(wstring packageHash) noexcept 
{
    if (packageHash.empty())
    {
        return;
    }

    auto localSettings{ ApplicationData::Current().LocalSettings() };
    JsonObject latestRollbackInfo;
    auto res{ localSettings.Values().TryLookup(LatestRollbackInfoKey) };
    if (res != nullptr)
    {
        auto infoString{ unbox_value<hstring>(res) };
        JsonObject::TryParse(infoString, latestRollbackInfo);
    }

    auto initialRollbackCount{ GetRollbackCountForPackage(packageHash, latestRollbackInfo) };
    auto count{ initialRollbackCount + 1 };
    auto currentTimeMillis{ clock::to_time_t(clock::now()) * 1000 };

    latestRollbackInfo.Insert(LatestRollbackCountKey, JsonValue::CreateNumberValue(count));
    latestRollbackInfo.Insert(LatestRollbackTimeKey, JsonValue::CreateNumberValue(currentTimeMillis));
    latestRollbackInfo.Insert(LatestRollbackPackageHashKey, JsonValue::CreateStringValue(packageHash));

    localSettings.Values().Insert(LatestRollbackInfoKey, box_value(latestRollbackInfo.Stringify()));
}

/*
 * This method is used to get information about the latest rollback.
 * This information will be used to decide whether the application
 * should ignore the update or not.
 */
void CodePushNativeModule::GetLatestRollbackInfo(ReactPromise<IJsonValue> promise) noexcept 
{
    auto localSettings{ ApplicationData::Current().LocalSettings() };
    auto res{ localSettings.Values().TryLookup(LatestRollbackInfoKey) };
    auto infoString{ unbox_value<hstring>(res) };
    JsonObject latestRollbackInfo;
    auto success{ JsonObject::TryParse(infoString, latestRollbackInfo) };
    if (success)
    {
        promise.Resolve(latestRollbackInfo);
    }
    else
    {
        promise.Resolve(JsonValue::CreateNullValue());
    }
}

/*
 * This method isn't publicly exposed via the "react-native-code-push"
 * module, and is only used internally to populate the LocalPackage.isFirstRun property.
 */
fire_and_forget CodePushNativeModule::IsFirstRun(wstring packageHash, ReactPromise<bool> promise) noexcept 
{
    auto isFirstRun = m_isFirstRunAfterUpdate
        && !packageHash.empty()
        && packageHash == co_await CodePushPackage::GetCurrentPackageHashAsync();

    promise.Resolve(isFirstRun);
}

/*
 * This method is the native side of the CodePush.notifyApplicationReady() method.
 */
void CodePushNativeModule::NotifyApplicationReady(ReactPromise<IJsonValue> promise) noexcept
{
    RemovePendingUpdate();
    promise.Resolve(JsonValue::CreateNullValue());
}

void CodePushNativeModule::Allow(ReactPromise<JSValue> promise) noexcept 
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

void CodePushNativeModule::ClearPendingRestart() noexcept 
{
    m_restartQueue.clear();
}

void CodePushNativeModule::Disallow(ReactPromise<JSValue> promise) noexcept
{
    CodePushUtils::Log(L"Disallowing restarts.");
    m_allowed = false;
    promise.Resolve(JSValue::Null);
}

/*
 * This method is the native side of the CodePush.restartApp() method.
 */
void CodePushNativeModule::RestartApp(bool onlyIfUpdateIsPending, ReactPromise<JSValue> promise) noexcept 
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
fire_and_forget CodePushNativeModule::ClearUpdates() noexcept 
{
    co_await CodePushPackage::ClearUpdatesAsync();
    RemovePendingUpdate();
    RemoveFailedUpdates();
}

// #pragma mark - JavaScript-exported module methods (Private)

/*
 * This method is the native side of the CodePush.downloadAndReplaceCurrentBundle()
 * method, which replaces the current bundle with the one downloaded from
 * removeBundleUrl. It is only to be used during tests and no-ops if the test
 * configuration flag is not set.
 */
fire_and_forget CodePushNativeModule::DownloadAndReplaceCurrentBundle(wstring remoteBundleUrl) noexcept 
{
    if (IsUsingTestConfiguration())
    {
        co_await CodePushPackage::DownloadAndReplaceCurrentBundleAsync(remoteBundleUrl);
    }
}

/*
 * This method is checks if a new status update exists (new version was installed,
 * or an update failed) and return its details (version label, status).
 */
fire_and_forget CodePushNativeModule::GetNewStatusReportAsync(ReactPromise<IJsonValue> promise) noexcept 
{
    if (needToReportRollback)
    {
        needToReportRollback = false;
        auto localSettings{ ApplicationData::Current().LocalSettings() };
        auto failedUpdatesData{ localSettings.Values().TryLookup(FailedUpdatesKey) };
        if (failedUpdatesData != nullptr)
        {
            auto failedUpdatesString{ unbox_value<hstring>(failedUpdatesData) };
            JsonArray failedUpdates;
            auto success{ JsonArray::TryParse(failedUpdatesString, failedUpdates) };
            if (success)
            {
                auto lastFailedPackage{ failedUpdates.GetObjectAt(failedUpdates.Size() - 1) };
                if (lastFailedPackage != nullptr)
                {
                    promise.Resolve(CodePushTelemetryManager::GetRollbackReport(lastFailedPackage));
                    co_return;
                }
            }
        }
    }
    else if (m_isFirstRunAfterUpdate)
    {
        auto currentPackage = co_await CodePushPackage::GetCurrentPackageAsync();
        promise.Resolve(CodePushTelemetryManager::GetUpdateReport(currentPackage));
        co_return;
    }
    else if (isRunningBinaryVersion)
    {
        auto appVersion{ m_codePushConfig.GetAppVersion() };
        wstring_view appVersionString{ appVersion.has_value() ? appVersion.value() : L"" };
        promise.Resolve(CodePushTelemetryManager::GetBinaryUpdateReport(appVersionString));
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

    promise.Resolve(JsonValue::CreateNullValue());
    co_return;
}

void CodePushNativeModule::RecordStatusReported(JsonObject statusReport) noexcept 
{
    CodePushTelemetryManager::RecordStatusReported(statusReport);
}

void CodePushNativeModule::SaveStatusReportForRetry(JsonObject statusReport) noexcept 
{
    CodePushTelemetryManager::SaveStatusReportForRetry(statusReport);
}

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