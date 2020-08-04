#include "pch.h"

#include "CodePush.h"
#include "CodePushUtils.h"
#include "CodePushPackage.h"
#include "App.h"

#include <winrt/Windows.Data.Json.h>

#include <exception>

#include "AutolinkedNativeModules.g.h"
#include "ReactPackageProvider.h"

using namespace winrt::Microsoft::ReactNative;
using namespace winrt::Windows::Data::Json;

const winrt::hstring PackageHashKey{ L"packageHash" };

void CodePush::CodePush::LoadBundle()
{
    //auto instanceManager = ReactApplication::InstanceSettings;
    
    // Get the latest JSBundle file

    // Change the local JSBundle path

    // Initialize after restart?
    /*
    m_context.UIDispatcherQueue().Post([host = m_host]() {
        host.ReloadInstance(); });
    */
    
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

    m_host.InstanceSettings().UIDispatcher().Post([host = m_host]() {
        host.ReloadInstance(); 
        //OutputDebugStringW(L"What's up?");
        //host.PackageProviders().Append(winrt::make<winrt::CodePushDemoAppCpp::implementation::ReactPackageProvider>());
    });
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

void CodePush::CodePush::GetNewStatusReport(ReactPromise<JSValue> promise) noexcept
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