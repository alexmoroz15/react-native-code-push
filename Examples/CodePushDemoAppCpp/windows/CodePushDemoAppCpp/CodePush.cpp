#include "pch.h"

#include "CodePush.h"
#include "CodePushUtils.h"
#include "CodePushPackage.h"
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
-(void)loadBundle
{
    // This needs to be async dispatched because the bridge is not set on init
    // when the app first starts, therefore rollbacks will not take effect.
    dispatch_async(dispatch_get_main_queue(), ^ {
        // If the current bundle URL is using http(s), then assume the dev
        // is debugging and therefore, shouldn't be redirected to a local
        // file (since Chrome wouldn't support it). Otherwise, update
        // the current bundle URL to point at the latest update
        if ([CodePush isUsingTestConfiguration] || ![super.bridge.bundleURL.scheme hasPrefix : @"http"]) {
            [super.bridge setValue : [CodePush bundleURL] forKey : @"bundleURL"] ;
        }

        [super.bridge reload];
        });
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
- (void)restartAppInternal:(BOOL)onlyIfUpdateIsPending
{
    if (_restartInProgress) {
        CPLog(@"Restart request queued until the current restart is completed.");
        [_restartQueue addObject:@(onlyIfUpdateIsPending)];
        return;
    } else if (!_allowed) {
        CPLog(@"Restart request queued until restarts are re-allowed.");
        [_restartQueue addObject:@(onlyIfUpdateIsPending)];
        return;
    }

    _restartInProgress = YES;
    if (!onlyIfUpdateIsPending || [[self class] isPendingUpdate:nil]) {
        [self loadBundle];
        CPLog(@"Restarting app.");
        return;
    }

    _restartInProgress = NO;
    if ([_restartQueue count] > 0) {
        BOOL buf = [_restartQueue valueForKey: @"@firstObject"];
        [_restartQueue removeObjectAtIndex:0];
        [self restartAppInternal:buf];
    }
}
        */

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

/*
 * This is native-side of the RemotePackage.download method
 */
fire_and_forget CodePush::CodePush::DownloadUpdateAsync(JsonObject updatePackage, bool notifyProgress, ReactPromise<JsonObject> promise) noexcept { co_return; }

/*
 * This is the native side of the CodePush.getConfiguration method. It isn't
 * currently exposed via the "react-native-code-push" module, and is used
 * internally only by the CodePush.checkForUpdate method in order to get the
 * app version, as well as the deployment key that was configured in the Info.plist file.
 */
void CodePush::CodePush::GetConfiguration(ReactPromise<JsonObject> promise) noexcept {}

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
void CodePush::CodePush::NotifyApplicationReady() noexcept {}

void CodePush::CodePush::Allow() noexcept {}

void CodePush::CodePush::ClearPendingRestart() noexcept {}

void CodePush::CodePush::Disallow() noexcept {}

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
void CodePush::CodePush::GetNewStatusReport(ReactPromise<JsonObject> promise) noexcept {}

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