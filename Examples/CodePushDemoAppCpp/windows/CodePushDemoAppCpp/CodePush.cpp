#include "pch.h"
#include "CodePush.h"
#include "CodePushUtils.h"

#include <exception>

using namespace winrt::Microsoft::ReactNative;

void CodePush::CodePush::LoadBundle()
{
    /*
    ClearLifeCycleEventListener();
    try
    {
        mCodePush.ClearDebugCacheIfNeeded(ResolveInstanceManager());
    }
    catch (...)
    {
        mCodePush.ClearDebugCacheIfNeeded(nullptr);
    }

    try 
    {
        auto instanceManager = ResolveInstanceManager();
        if (!instanceManager)
        {
            return;
        }

        auto latestJSBundleFile = mCodePush.GetJSBundleFileInternal(mCodePush.GetAssentsBundleFileName());

        // Handler???
    }
    catch (...)
    {
    }
    */

    /*
    private void loadBundle() {
        clearLifecycleEventListener();
        try {
            mCodePush.clearDebugCacheIfNeeded(resolveInstanceManager());
        }
        catch (Exception e) {
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
            new Handler(Looper.getMainLooper()).post(new Runnable(){
                @Override
                public void run() {
                    try {
                        // We don't need to resetReactRootViews anymore 
                        // due the issue https://github.com/facebook/react-native/issues/14533
                        // has been fixed in RN 0.46.0
                        //resetReactRootViews(instanceManager);

                        instanceManager.recreateReactContextInBackground();
                        mCodePush.initializeUpdateAfterRestart();
                    }
 catch (Exception e) {
     // The recreation method threw an unknown exception
     // so just simply fallback to restarting the Activity (if it exists)
     loadBundleLegacy();
 }
}
                });

        }
        catch (Exception e) {
            // Our reflection logic failed somewhere
            // so fall back to restarting the Activity (if it exists)
            CodePushUtils.log("Failed to load the bundle, falling back to restarting the Activity (if it exists). " + e.getMessage());
            loadBundleLegacy();
        }
    }
    */
}

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
    if (!onlyIfUpdateIsPending /* || mSettingsManager.isPendingUpdate()*/)
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
	try
	{
	    RestartAppInternal(onlyIfUpdateIsPending);
	    promise.Resolve(JSValue::Null);
	}
	catch(std::exception e)
	{
		CodePushUtils::Log(e);
		promise.Reject(e.what());
	}
}

void CodePush::CodePush::NotifyApplicationReady(ReactPromise<JSValue> promise)
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
