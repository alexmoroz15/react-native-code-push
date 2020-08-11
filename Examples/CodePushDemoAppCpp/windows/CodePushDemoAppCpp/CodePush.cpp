#include "pch.h"

#include "CodePush.h"
#include "CodePushUtils.h"
#include "CodePushPackage.h"
#include "App.h"

#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.Web.Http.h>
#include <winrt/Windows.Storage.Streams.h>

#include <exception>
#include <filesystem>

#include "AutolinkedNativeModules.g.h"
#include "ReactPackageProvider.h"

using namespace winrt;
using namespace Microsoft::ReactNative;
using namespace Windows::Data::Json;
using namespace Windows::Web::Http;
using namespace Windows::Storage::Streams;
using namespace Windows::Foundation;

using namespace std::filesystem;

const hstring PackageHashKey{ L"packageHash" };

bool IsPackageBundleLatest(IJsonValue packageMetadata)
{
    // idk
    return false;
}
/*
private boolean isPackageBundleLatest(JSONObject packageMetadata) {
        try {
            Long binaryModifiedDateDuringPackageInstall = null;
            String binaryModifiedDateDuringPackageInstallString = packageMetadata.optString(CodePushConstants.BINARY_MODIFIED_TIME_KEY, null);
            if (binaryModifiedDateDuringPackageInstallString != null) {
                binaryModifiedDateDuringPackageInstall = Long.parseLong(binaryModifiedDateDuringPackageInstallString);
            }
            String packageAppVersion = packageMetadata.optString("appVersion", null);
            long binaryResourcesModifiedTime = this.getBinaryResourcesModifiedTime();
            return binaryModifiedDateDuringPackageInstall != null &&
                    binaryModifiedDateDuringPackageInstall == binaryResourcesModifiedTime &&
                    (isUsingTestConfiguration() || sAppVersion.equals(packageAppVersion));
        } catch (NumberFormatException e) {
            throw new CodePushUnknownException("Error in reading binary modified date from package metadata", e);
        }
    }
*/

IAsyncOperation<hstring> CodePush::CodePush::GetJSBundleFile()
{
    return GetJSBundleFile(DefaultJSBundleName);
}

IAsyncOperation<hstring> CodePush::CodePush::GetJSBundleFile(hstring assetsBundleFileName)
{
    m_assetsBundleFileName = assetsBundleFileName;
    auto binaryJSBundleUrl = AssetsBundlePrefix +  m_assetsBundleFileName;
    //auto binaryJSBundleUrl = m_assetsBundleFileName;
    auto packageFilePath = co_await CodePushPackage::GetCurrentPackageBundlePath(m_assetsBundleFileName);
    if (packageFilePath.empty())
    {
        //There has not been any downloaded updates
        // log stuff
        CodePushUtils::LogBundleUrl(packageFilePath.c_str());
        co_return binaryJSBundleUrl;
    }

    auto packageMetadata = co_await CodePushPackage::GetCurrentPackage();
    if (IsPackageBundleLatest(packageMetadata))
    {
        CodePushUtils::LogBundleUrl(packageFilePath.c_str());
        co_return packageFilePath;
    }
    else
    {
        CodePushUtils::LogBundleUrl(binaryJSBundleUrl.c_str());
        co_return binaryJSBundleUrl;
    }
}

/*
public String getJSBundleFileInternal(String assetsBundleFileName) {
        this.mAssetsBundleFileName = assetsBundleFileName;
        String binaryJsBundleUrl = CodePushConstants.ASSETS_BUNDLE_PREFIX + assetsBundleFileName;

        String packageFilePath = null;
        try {
            packageFilePath = mUpdateManager.getCurrentPackageBundlePath(this.mAssetsBundleFileName);
        } catch (CodePushMalformedDataException e) {
            // We need to recover the app in case 'codepush.json' is corrupted
            CodePushUtils.log(e.getMessage());
            clearUpdates();
        }

        if (packageFilePath == null) {
            // There has not been any downloaded updates.
            CodePushUtils.logBundleUrl(binaryJsBundleUrl);
            sIsRunningBinaryVersion = true;
            return binaryJsBundleUrl;
        }

        JSONObject packageMetadata = this.mUpdateManager.getCurrentPackage();
        if (isPackageBundleLatest(packageMetadata)) {
            CodePushUtils.logBundleUrl(packageFilePath);
            sIsRunningBinaryVersion = false;
            return packageFilePath;
        } else {
            // The binary version is newer.
            this.mDidUpdate = false;
            if (!this.mIsDebugMode || hasBinaryVersionChanged(packageMetadata)) {
                this.clearUpdates();
            }

            CodePushUtils.logBundleUrl(binaryJsBundleUrl);
            sIsRunningBinaryVersion = true;
            return binaryJsBundleUrl;
        }
    }
*/
/*
+ (NSURL *)bundleURLForResource:(NSString *)resourceName
                  withExtension:(NSString *)resourceExtension
                   subdirectory:(NSString *)resourceSubdirectory
                         bundle:(NSBundle *)resourceBundle
{
    bundleResourceName = resourceName;
    bundleResourceExtension = resourceExtension;
    bundleResourceSubdirectory = resourceSubdirectory;
    bundleResourceBundle = resourceBundle;

    [self ensureBinaryBundleExists];

    NSString *logMessageFormat = @"Loading JS bundle from %@";

    NSError *error;
    NSString *packageFile = [CodePushPackage getCurrentPackageBundlePath:&error];
    NSURL *binaryBundleURL = [self binaryBundleURL];

    if (error || !packageFile) {
        CPLog(logMessageFormat, binaryBundleURL);
        isRunningBinaryVersion = YES;
        return binaryBundleURL;
    }

    NSString *binaryAppVersion = [[CodePushConfig current] appVersion];
    NSDictionary *currentPackageMetadata = [CodePushPackage getCurrentPackage:&error];
    if (error || !currentPackageMetadata) {
        CPLog(logMessageFormat, binaryBundleURL);
        isRunningBinaryVersion = YES;
        return binaryBundleURL;
    }

    NSString *packageDate = [currentPackageMetadata objectForKey:BinaryBundleDateKey];
    NSString *packageAppVersion = [currentPackageMetadata objectForKey:AppVersionKey];

    if ([[CodePushUpdateUtils modifiedDateStringOfFileAtURL:binaryBundleURL] isEqualToString:packageDate] && ([CodePush isUsingTestConfiguration] ||[binaryAppVersion isEqualToString:packageAppVersion])) {
        // Return package file because it is newer than the app store binary's JS bundle
        NSURL *packageUrl = [[NSURL alloc] initFileURLWithPath:packageFile];
        CPLog(logMessageFormat, packageUrl);
        isRunningBinaryVersion = NO;
        return packageUrl;
    } else {
        BOOL isRelease = NO;
#ifndef DEBUG
        isRelease = YES;
#endif

        if (isRelease || ![binaryAppVersion isEqualToString:packageAppVersion]) {
            [CodePush clearUpdates];
        }

        CPLog(logMessageFormat, binaryBundleURL);
        isRunningBinaryVersion = YES;
        return binaryBundleURL;
    }
}
*/

bool CodePush::CodePush::IsUsingTestConfiguration()
{
    return _testConfigurationFlag;
}

void CodePush::CodePush::SetUsingTestConfiguration(bool shouldUseTestConfiguration)
{
    _testConfigurationFlag = shouldUseTestConfiguration;
}

fire_and_forget CodePush::CodePush::LoadBundle()
{
    /*
    auto bundleRootPath = m_host.InstanceSettings().BundleRootPath();
    OutputDebugStringW((L"BundleRootPath: " + bundleRootPath + L"\n").c_str());
    auto debugBundlePath = m_host.InstanceSettings().DebugBundlePath();
    OutputDebugStringW((L"DebugBundlePath: " + debugBundlePath + L"\n").c_str());
    auto jsBundleFile = m_host.InstanceSettings().JavaScriptBundleFile();
    OutputDebugStringW((L"JavaScriptBundleFile: " + jsBundleFile + L"\n").c_str());
    auto jsMainModuleName = m_host.InstanceSettings().JavaScriptMainModuleName();
    OutputDebugStringW((L"JavaScriptMainModuleName: " + jsMainModuleName + L"\n").c_str());
    auto byteCodeFileUri = m_host.InstanceSettings().ByteCodeFileUri();
    OutputDebugStringW((L"ByteCodeFileUri: " + byteCodeFileUri + L"\n").c_str());
    */

    //m_host.InstanceSettings().JavaScriptBundleFile(co_await GetJSBundleFile());
    /*
    if (IsUsingTestConfiguration() ||
        std::wstring(m_host.InstanceSettings().JavaScriptBundleFile().c_str()).rfind(L"http", 0) == std::wstring::npos)
    {
        m_host.InstanceSettings().JavaScriptBundleFile(co_await GetJSBundleFile());
    }
    */

    m_host.InstanceSettings().UIDispatcher().Post([host = m_host]() {
        host.ReloadInstance();
    });
    co_return;
}

/*
    private void loadBundle() {
        clearLifecycleEventListener();
        try {
            mCodePush.clearDebugCacheIfNeeded(resolveInstanceManager());
        } catch(Exception e) {
            // If we got error in out reflection we should clear debug cache anyway.
            mCodePush.clearDebugCacheIfNeeded(null);
        }

        try {
            // #1) Get the ReactInstanceManager instance, which is what includes the
            //     logic to reload the current React context.
            final ReactInstanceManager instanceManager = resolveInstanceManager();
            if (instanceManager == null) {
                return;
            }

            String latestJSBundleFile = mCodePush.getJSBundleFileInternal(mCodePush.getAssetsBundleFileName());

            // #2) Update the locally stored JS bundle file path
            setJSBundle(instanceManager, latestJSBundleFile);

            // #3) Get the context creation method and fire it on the UI thread (which RN enforces)
            new Handler(Looper.getMainLooper()).post(new Runnable() {
                @Override
                public void run() {
                    try {
                        // We don't need to resetReactRootViews anymore
                        // due the issue https://github.com/facebook/react-native/issues/14533
                        // has been fixed in RN 0.46.0
                        //resetReactRootViews(instanceManager);

                        instanceManager.recreateReactContextInBackground();
                        mCodePush.initializeUpdateAfterRestart();
                    } catch (Exception e) {
                        // The recreation method threw an unknown exception
                        // so just simply fallback to restarting the Activity (if it exists)
                        loadBundleLegacy();
                    }
                }
            });

        } catch (Exception e) {
            // Our reflection logic failed somewhere
            // so fall back to restarting the Activity (if it exists)
            CodePushUtils.log("Failed to load the bundle, falling back to restarting the Activity (if it exists). " + e.getMessage());
            loadBundleLegacy();
        }
    }
*/

/*
- (void)loadBundle
{
    // This needs to be async dispatched because the bridge is not set on init
    // when the app first starts, therefore rollbacks will not take effect.
    dispatch_async(dispatch_get_main_queue(), ^{
        // If the current bundle URL is using http(s), then assume the dev
        // is debugging and therefore, shouldn't be redirected to a local
        // file (since Chrome wouldn't support it). Otherwise, update
        // the current bundle URL to point at the latest update
        if ([CodePush isUsingTestConfiguration] || ![super.bridge.bundleURL.scheme hasPrefix:@"http"]) {
            [super.bridge setValue:[CodePush bundleURL] forKey:@"bundleURL"];
        }

        [super.bridge reload];
    });
}
*/

void CodePush::CodePush::Initialize(ReactContext const& reactContext) noexcept
{
    /*
    auto res = reactContext.Properties().Handle().Get(ReactPropertyBagHelper::GetName(nullptr, L"MyReactNativeHost"));
    m_host = res.as<ReactNativeHost>();
    */
    //reactContext.Properties().Get(ReactPropertyBag::Get(ReactPropertyBagHelper::GetName(nullptr, L"MyReactNativeHost"), nullptr), nullptr);
    //reactContext.Properties().Get(ReactPropertyBagHelper::GetName(nullptr, L"MyReactNativeHost"), nullptr);
    //reactContext.Properties().Get(ReactPropertyBag::Handle
    m_host = g_host;

    //file = L"CodePushHash";
    /*
    auto file = co_await StorageFile::GetFileFromPathAsync(updateMetadataFilePath.c_str());
    auto content = co_await FileIO::ReadTextAsync(file);
    auto json = JsonObject::Parse(content);
    */
    //m_BinaryContentsHash;
    //m_instance = g_instanceSettings;
}

bool CodePush::CodePush::IsPendingUpdate(winrt::hstring&& packageHash)
{
    // I'm not sure if UWP has a similar local dedicated storage object

    // NSUserDefaults* preferences = [NSUserDefaults standardUserDefaults];
    // NSDictionary* pendingUpdate = [preferences objectForKey : PendingUpdateKey];
    auto pendingUpdate = JsonObject();
    pendingUpdate.SetNamedValue(PendingUpdateIsLoadingKey, JsonValue::CreateBooleanValue(false));
    pendingUpdate.SetNamedValue(PendingUpdateHashKey, JsonValue::CreateStringValue(L"asdf"));

    try
    {
        bool updateIsPending = pendingUpdate.Size() && 
            !pendingUpdate.GetNamedBoolean(PendingUpdateIsLoadingKey) && 
            (!packageHash.size() || pendingUpdate.GetNamedString(PendingUpdateHashKey) == packageHash);

        return updateIsPending;
    }
    catch (...)
    {
        // throw new CodePushUnknownException("Unable to read pending update metadata in isPendingUpdate.", e);
        throw;
    }
}

std::string GetServerUrl()
{
    return "";
}

std::string GetDeploymentKey()
{
    return "";
}

std::string GetAppVersion()
{
    return "";
}

void CodePush::CodePush::GetConfiguration(ReactPromise<JSValue>&& promise) noexcept
{
    JSValueObject configMap = JSValueObject{};
    configMap["appVersion"] = GetAppVersion();
    configMap["deploymentKey"] = GetDeploymentKey();
    configMap["serverUrl"] = GetServerUrl();

    if (!m_BinaryContentsHash.empty())
    {
        configMap["packageHash"] = m_BinaryContentsHash;
    }

    
    promise.Resolve(std::move(configMap));
}
/*
@ReactMethod
    public void getConfiguration(Promise promise) {
        try {
            WritableMap configMap =  Arguments.createMap();
            configMap.putString("appVersion", mCodePush.getAppVersion());
            configMap.putString("clientUniqueId", mClientUniqueId);
            configMap.putString("deploymentKey", mCodePush.getDeploymentKey());
            configMap.putString("serverUrl", mCodePush.getServerUrl());

            // The binary hash may be null in debug builds
            if (mBinaryContentsHash != null) {
                configMap.putString(CodePushConstants.PACKAGE_HASH_KEY, mBinaryContentsHash);
            }

            promise.resolve(configMap);
        } catch(CodePushUnknownException e) {
            CodePushUtils.log(e);
            promise.reject(e);
        }
    }
*/

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

void CodePush::CodePush::GetNewStatusReport(ReactPromise<JSValue>&& promise) noexcept
{
    promise.Resolve(JSValue::Null);
}

winrt::fire_and_forget CodePush::CodePush::GetUpdateMetadata(CodePushUpdateState updateState, ReactPromise<JSValue> promise) noexcept
{
    try
    {
        auto currentPackage = co_await CodePushPackage::GetCurrentPackage();
        if (currentPackage.ValueType() == JsonValueType::Null)
        {
            promise.Resolve(JSValue::Null);
            co_return;
        }

        // ...
        auto currentUpdateIsPending = IsPendingUpdate(currentPackage.GetObject().GetNamedString(PackageHashKey));

        //promise.Resolve(currentPackage);
        promise.Resolve(JSValue::Null);
    }
    catch (std::exception e)
    {
        promise.Reject(e.what());
    }
    //m_host.InstanceSettings().BundleRootPath();
}

/*
@ReactMethod
    public void getUpdateMetadata(final int updateState, final Promise promise) {
        AsyncTask<Void, Void, Void> asyncTask = new AsyncTask<Void, Void, Void>() {
            @Override
            protected Void doInBackground(Void... params) {
                try {
                    JSONObject currentPackage = mUpdateManager.getCurrentPackage();

                    if (currentPackage == null) {
                        promise.resolve(null);
                        return null;
                    }

                    Boolean currentUpdateIsPending = false;

                    if (currentPackage.has(CodePushConstants.PACKAGE_HASH_KEY)) {
                        String currentHash = currentPackage.optString(CodePushConstants.PACKAGE_HASH_KEY, null);
                        currentUpdateIsPending = mSettingsManager.isPendingUpdate(currentHash);
                    }

                    if (updateState == CodePushUpdateState.PENDING.getValue() && !currentUpdateIsPending) {
                        // The caller wanted a pending update
                        // but there isn't currently one.
                        promise.resolve(null);
                    } else if (updateState == CodePushUpdateState.RUNNING.getValue() && currentUpdateIsPending) {
                        // The caller wants the running update, but the current
                        // one is pending, so we need to grab the previous.
                        JSONObject previousPackage = mUpdateManager.getPreviousPackage();

                        if (previousPackage == null) {
                            promise.resolve(null);
                            return null;
                        }

                        promise.resolve(CodePushUtils.convertJsonObjectToWritable(previousPackage));
                    } else {
                        // The current package satisfies the request:
                        // 1) Caller wanted a pending, and there is a pending update
                        // 2) Caller wanted the running update, and there isn't a pending
                        // 3) Caller wants the latest update, regardless if it's pending or not
                        if (mCodePush.isRunningBinaryVersion()) {
                            // This only matters in Debug builds. Since we do not clear "outdated" updates,
                            // we need to indicate to the JS side that somehow we have a current update on
                            // disk that is not actually running.
                            CodePushUtils.setJSONValueForKey(currentPackage, "_isDebugOnly", true);
                        }

                        // Enable differentiating pending vs. non-pending updates
                        CodePushUtils.setJSONValueForKey(currentPackage, "isPending", currentUpdateIsPending);
                        promise.resolve(CodePushUtils.convertJsonObjectToWritable(currentPackage));
                    }
                } catch (CodePushMalformedDataException e) {
                    // We need to recover the app in case 'codepush.json' is corrupted
                    CodePushUtils.log(e.getMessage());
                    clearUpdates();
                    promise.resolve(null);
                } catch(CodePushUnknownException e) {
                    CodePushUtils.log(e);
                    promise.reject(e);
                }

                return null;
            }
        };

        asyncTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }
*/

void CodePush::CodePush::RestartAppInternal(bool onlyIfUpdateIsPending)
{
    if (_restartInProgress)
    {
        CodePushUtils::Log(L"Restart request queued until the current restart is completed");
        _restartQueue.push_back(onlyIfUpdateIsPending);
        return;
    }
    else if (!_allowed)
    {
        CodePushUtils::Log(L"Restart request queued until restarts are re-allowed");
        _restartQueue.push_back(onlyIfUpdateIsPending);
        return;
    }
    
    _restartInProgress = true;
    if (!onlyIfUpdateIsPending || IsPendingUpdate(L""))
    {
        LoadBundle();
        CodePushUtils::Log(L"Restarting app");
        return;
    }
    
    _restartInProgress = false;
    if (_restartQueue.size() > 0)
    {
        auto buf = _restartQueue[0];
        _restartQueue.erase(_restartQueue.begin());
        RestartAppInternal(buf);
    }
}

void CodePush::CodePush::RestartApp(bool onlyIfUpdateIsPending, ReactPromise<JSValue>&& promise) noexcept
{
	RestartAppInternal(onlyIfUpdateIsPending);
	promise.Resolve(JSValue::Null);
}

void CodePush::CodePush::NotifyApplicationReady(ReactPromise<JSValue>&& promise) noexcept
{
    try {
        //mSettingsManager.removePendingUpdate();
        promise.Resolve(JSValue::Null);
    }
    catch (std::exception/*CodePushUnknownException*/ e) {
        CodePushUtils::Log(e);
        promise.Reject(e.what());
    }
}

void CodePush::CodePush::Allow(ReactPromise<JSValue>&& promise) noexcept
{
    CodePushUtils::Log(L"Re-allowing restarts");
    _allowed = true;

    if (_restartQueue.size() > 0)
    {
        CodePushUtils::Log(L"Executing pending restart");
        auto buf = _restartQueue[0];
        _restartQueue.erase(_restartQueue.begin());
        RestartAppInternal(buf);
    }

    promise.Resolve(JSValue::Null);
}

/*
@ReactMethod
    public void allow(Promise promise) {
        CodePushUtils.log("Re-allowing restarts");
        this._allowed = true;

        if (_restartQueue.size() > 0) {
            CodePushUtils.log("Executing pending restart");
            boolean buf = this._restartQueue.get(0);
            this._restartQueue.remove(0);
            this.restartAppInternal(buf);
        }

        promise.resolve(null);
        return;
    }
*/

void CodePush::CodePush::Disallow(ReactPromise<JSValue>&& promise) noexcept
{
    CodePushUtils::Log(L"Disallowing restarts");
    _allowed = false;
    promise.Resolve(JSValue::Null);
}

/*
@ReactMethod
    public void disallow(Promise promise) {
        CodePushUtils.log("Disallowing restarts");
        this._allowed = false;
        promise.resolve(null);
        return;
    }
*/


const std::wstring BinaryModifiedTimeKey = L"";

long GetBinaryResourcesModifiedTime()
{
    std::wstring packagename = L"PackageName";
    int codePushApkBuildTimeId = -1;
    std::wstring CodePushApkBuildTime = L"";
    //return static_cast<long>(CodePushApkBuildTime);
    return -1;
}
/*
long getBinaryResourcesModifiedTime() {
        try {
            String packageName = this.mContext.getPackageName();
            int codePushApkBuildTimeId = this.mContext.getResources().getIdentifier(CodePushConstants.CODE_PUSH_APK_BUILD_TIME_KEY, "string", packageName);
            // replace double quotes needed for correct restoration of long value from strings.xml
            // https://github.com/microsoft/cordova-plugin-code-push/issues/264
            String codePushApkBuildTime = this.mContext.getResources().getString(codePushApkBuildTimeId).replaceAll("\"","");
            return Long.parseLong(codePushApkBuildTime);
        } catch (Exception e) {
            throw new CodePushUnknownException("Error in getting binary resources modified time", e);
        }
    }
*/

void CodePush::CodePush::DownloadUpdate(JSValue&& updatePackage, bool notifyProgress, ReactPromise<JSValue>&& promise) noexcept
{
    JsonObject mutableUpdatePackage;
    
    //updatePackage[BinaryModifiedTimeKey] = GetBinaryResourcesModifiedTime();
    /*
    HttpClient foo;
    auto bar = foo.GetInputStreamAsync(Uri(L"http://foo"));
    auto a = bar.Progress();
    //a(auto b, auto c);
    auto b = bar.Status();
    auto c = bar.get();
    auto d = Buffer{ nullptr };
    c.ReadAsync(d, 8, InputStreamOptions()).Completed([](){
    
    });
    */
}

/*
@ReactMethod
    public void downloadUpdate(final ReadableMap updatePackage, final boolean notifyProgress, final Promise promise) {
        AsyncTask<Void, Void, Void> asyncTask = new AsyncTask<Void, Void, Void>() {
            @Override
            protected Void doInBackground(Void... params) {
                try {
                    JSONObject mutableUpdatePackage = CodePushUtils.convertReadableToJsonObject(updatePackage);
                    CodePushUtils.setJSONValueForKey(mutableUpdatePackage, CodePushConstants.BINARY_MODIFIED_TIME_KEY, "" + mCodePush.getBinaryResourcesModifiedTime());
                    mUpdateManager.downloadPackage(mutableUpdatePackage, mCodePush.getAssetsBundleFileName(), new DownloadProgressCallback() {
                        private boolean hasScheduledNextFrame = false;
                        private DownloadProgress latestDownloadProgress = null;

                        @Override
                        public void call(DownloadProgress downloadProgress) {
                            if (!notifyProgress) {
                                return;
                            }

                            latestDownloadProgress = downloadProgress;
                            // If the download is completed, synchronously send the last event.
                            if (latestDownloadProgress.isCompleted()) {
                                dispatchDownloadProgressEvent();
                                return;
                            }

                            if (hasScheduledNextFrame) {
                                return;
                            }

                            hasScheduledNextFrame = true;
                            getReactApplicationContext().runOnUiQueueThread(new Runnable() {
                                @Override
                                public void run() {
                                    ReactChoreographer.getInstance().postFrameCallback(ReactChoreographer.CallbackType.TIMERS_EVENTS, new ChoreographerCompat.FrameCallback() {
                                        @Override
                                        public void doFrame(long frameTimeNanos) {
                                            if (!latestDownloadProgress.isCompleted()) {
                                                dispatchDownloadProgressEvent();
                                            }

                                            hasScheduledNextFrame = false;
                                        }
                                    });
                                }
                            });
                        }

                        public void dispatchDownloadProgressEvent() {
                            getReactApplicationContext()
                                    .getJSModule(DeviceEventManagerModule.RCTDeviceEventEmitter.class)
                                    .emit(CodePushConstants.DOWNLOAD_PROGRESS_EVENT_NAME, latestDownloadProgress.createWritableMap());
                        }
                    }, mCodePush.getPublicKey());

                    JSONObject newPackage = mUpdateManager.getPackage(CodePushUtils.tryGetString(updatePackage, CodePushConstants.PACKAGE_HASH_KEY));
                    promise.resolve(CodePushUtils.convertJsonObjectToWritable(newPackage));
                } catch (CodePushInvalidUpdateException e) {
                    CodePushUtils.log(e);
                    mSettingsManager.saveFailedUpdate(CodePushUtils.convertReadableToJsonObject(updatePackage));
                    promise.reject(e);
                } catch (IOException | CodePushUnknownException e) {
                    CodePushUtils.log(e);
                    promise.reject(e);
                }

                return null;
            }
        };

        asyncTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }
*/