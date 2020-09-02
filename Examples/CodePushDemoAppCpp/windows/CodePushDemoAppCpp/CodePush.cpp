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

const hstring PackageHashKey{ L"packageHash" };

bool IsPackageBundleLatest(IJsonValue packageMetadata)
{
    // idk
    return false;
}

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

    //auto installedLocation{ Windows::ApplicationModel::Package::Current().InstalledLocation() };
    //m_host.InstanceSettings().BundleRootPath(installedLocation.Path() + L"\\Assets\\");
    //m_host.InstanceSettings().JavaScriptBundleFile(L"index.windows");
    m_host.InstanceSettings().UIDispatcher().Post([host = m_host]() {
        host.ReloadInstance();
    });
    co_return;
}

void CodePush::CodePush::Initialize(ReactContext const& reactContext) noexcept
{
    m_host = g_host;
}

bool CodePush::CodePush::IsPendingUpdate(winrt::hstring&& packageHash)
{
    return false;
}

void CodePush::CodePush::GetConfiguration(ReactPromise<JSValue>&& promise) noexcept
{
    JSValueObject configMap = JSValueObject{};
    configMap["appVersion"] = "1.0.0";
    configMap["deploymentKey"] = "BJwawsbtm8a1lTuuyN0GPPXMXCO1oUFtA_jJS";
    configMap["serverUrl"] = "https://codepush.appcenter.ms/";
    promise.Resolve(std::move(configMap));
}

void CodePush::CodePush::GetNewStatusReport(ReactPromise<JSValue>&& promise) noexcept
{
    promise.Resolve(JSValue::Null);
}

winrt::fire_and_forget CodePush::CodePush::GetUpdateMetadata(CodePushUpdateState updateState, ReactPromise<JSValue> promise) noexcept
{
    promise.Resolve(JSValue::Null);
    co_return;
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

void CodePush::CodePush::Disallow(ReactPromise<JSValue>&& promise) noexcept
{
    CodePushUtils::Log(L"Disallowing restarts");
    _allowed = false;
    promise.Resolve(JSValue::Null);
}

winrt::fire_and_forget CodePush::CodePush::DownloadUpdate(JSValueObject updatePackage, bool notifyProgress, ReactPromise<JSValue> promise) noexcept
{
    //JsonObject mutableUpdatePackage = {};

    auto downloadUrl{ updatePackage["downloadUrl"].AsString() };
    uint32_t BufferSize{ 1024 };

    HttpClient client;
    auto headers{ client.DefaultRequestHeaders() };
    headers.Append(L"Accept-Encoding", L"identity");

    auto storageFolder{ Windows::Storage::ApplicationData::Current().LocalFolder() };
    auto downloadFile{ co_await storageFolder.CreateFileAsync(L"download.zip", Windows::Storage::CreationCollisionOption::ReplaceExisting) };

    auto inputStream{ co_await client.GetInputStreamAsync(Uri(winrt::to_hstring(downloadUrl))) };
    auto outputStream{ co_await downloadFile.OpenAsync(Windows::Storage::FileAccessMode::ReadWrite) };

    //bool first{ false };
    //bool isZip{ false };
    bool isZip{ true };
    /*
    DataReader dr{ inputStream };
    dr.LoadAsync(4);
    auto header{ dr.ReadInt32() };
    isZip = header == 0x504b0304;
    dr.DetachStream();
    dr.Close();
    */
    for (;;)
    {
        auto outputBuffer{ co_await inputStream.ReadAsync(Buffer{ BufferSize }, BufferSize, InputStreamOptions::None) };
        if (outputBuffer.Length() == 0)
        {
            break;
        }
        co_await outputStream.WriteAsync(outputBuffer);
    }

    inputStream.Close();
    outputStream.Close();
    /*
    auto inputStream2{ co_await downloadFile.OpenAsync(Windows::Storage::FileAccessMode::Read) };
    DataReader dataReader{ inputStream2 };
    dataReader.LoadAsync(4);
    auto header{ dataReader.ReadInt32() };
    isZip = header == 0x504b0304;
    inputStream2.Close();
    */
    if (isZip)
    {
        auto unzippedFolderName{ L"unzipped" };
        auto unzippedFolder = co_await storageFolder.CreateFolderAsync(unzippedFolderName, CreationCollisionOption::ReplaceExisting);
        /*
        auto p{ path(to_string(storageFolder.Path())) / "download.zip" };
        auto n{ to_string(hstring(p.wstring())).c_str() };
        */
        auto zipName{ (to_string(storageFolder.Path()) + "\\download.zip") };

        mz_bool status;
        mz_zip_archive zip_archive;
        mz_zip_zero_struct(&zip_archive);

        status = mz_zip_reader_init_file(&zip_archive, zipName.c_str(), 0);
        auto numFiles{ mz_zip_reader_get_num_files(&zip_archive) };

        for (mz_uint i = 0; i < numFiles; i++)
        {
            mz_zip_archive_file_stat file_stat;
            status = mz_zip_reader_file_stat(&zip_archive, i, &file_stat);
            if (mz_zip_reader_is_file_a_directory(&zip_archive, i))
            {
                //StorageFolder::CreateFolderAsync((path(storageFolder.Path().c_str()) / unzippedName / file_stat.m_filename).c_str());
            }
            else
            {
                auto fileName{ file_stat.m_filename };
                auto filePath{ path(fileName) };
                auto filePathName{ filePath.filename() };
                auto filePathNameString{ filePathName.string() };

                // Current issue: cstdio is only allowed for read operations. 
                // I will have to use winrt async operations to write to a file.

                //auto pFile{ fopen(filePathNameString.c_str(), "w") };
                /*
                FILE* pFile{ nullptr };
                auto e{ fopen_s(&pFile, filePathNameString.c_str(), "r") };

                const UINT32 bufferSize{ 1024 };
                char pBuf[bufferSize];
                auto e2{ strerror_s(pBuf, bufferSize, e) };

                status = mz_zip_reader_extract_to_cfile(&zip_archive, i, pFile, 0);
                fclose(pFile);
                */
                //status = mz_zip_reader_extract_to_file(&zip_archive, i, "bla", 0);
                //auto entryFile{ co_await unzippedFolder.CreateFileAsync(to_hstring(filePathNameString), CreationCollisionOption::ReplaceExisting) };
                /*
                auto stream{ co_await entryFile.OpenAsync(FileAccessMode::ReadWrite) };
                auto os{ stream.GetOutputStreamAt(0) };
                DataWriter dw{ os };
                */
            }
        }

        status = mz_zip_reader_end(&zip_archive);
    }
    else
    {
        // Rename the file
        co_await downloadFile.MoveAsync(co_await downloadFile.GetParentAsync(), L"index.windows.bundle", Windows::Storage::NameCollisionOption::ReplaceExisting);
    }

    promise.Resolve(JSValue::Null);
    co_return;

    //updatePackage[BinaryModifiedTimeKey] = GetBinaryResourcesModifiedTime();
}

void unzip(StorageFile& zipFile, StorageFolder& destinationFolder)
{

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

void CodePush::CodePush::IsFailedUpdate(std::wstring packageHash, ReactPromise<bool> promise) noexcept
{
    promise.Resolve(false);
}