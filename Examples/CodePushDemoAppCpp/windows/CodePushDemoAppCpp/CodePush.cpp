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
#include <sstream>

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
    m_host.InstanceSettings().UIDispatcher().Post([host = m_host]() {
        host.ReloadInstance();
    });
    co_return;
}

void CodePush::CodePush::Initialize(ReactContext const& reactContext) noexcept
{
    m_context = reactContext;
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

/*
IAsyncOperation<StorageFile> CreateFileFromPathAsync(StorageFolder& rootFolder, path& relPath)
{
    while (relPath.has_root_directory())
    {
        auto root{ relPath.root_name() };
        auto root_string{ root.string() };

        IStorageItem item{ co_await rootFolder.TryGetItemAsync(to_hstring(root_string)) };
        if (item == NULL)
        {
            auto folder{ co_await rootFolder.CreateFolderAsync(to_hstring(root_string)) };
        }
        else if (item.IsOfType(StorageItemTypes::Folder))
        {
            //auto folder{ static_cast<StorageFolder>(item) };
            auto folder{ co_await rootFolder.GetFolderAsync(item.Name()) };
        }
        else // item.IsOfType(StorageItemTypes::File)
        {
            OutputDebugStringW(L"This isn't supposed to happen\n");
        }
    }
    co_return co_await rootFolder.CreateFileAsync(L"foo");
}
*/

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

            auto entryFile{ co_await destination.CreateFileAsync(to_hstring(filePathNameString), CreationCollisionOption::ReplaceExisting) };
            auto stream{ co_await entryFile.OpenAsync(FileAccessMode::ReadWrite) };
            auto os{ stream.GetOutputStreamAt(0) };
            DataWriter dw{ os };

            const auto arrBufSize = 8 * 1024;
            std::array<uint8_t, arrBufSize> arrBuf;

            mz_zip_reader_extract_iter_state* pState = mz_zip_reader_extract_iter_new(&zip_archive, i, 0);
            UINT32 bytesRead{ 0 };
            while (bytesRead = mz_zip_reader_extract_iter_read(pState, static_cast<void*>(arrBuf.data()), arrBuf.size()))
            {
                array_view<const uint8_t> view{ arrBuf.data(), arrBuf.data() + bytesRead };
                dw.WriteBytes(view);
            }
            status = mz_zip_reader_extract_iter_free(pState);
            assert(status);

            auto bar{ co_await dw.StoreAsync() };
            auto bla{ co_await dw.FlushAsync() };

            dw.Close();
            os.Close();
            stream.Close();
        }
    }

    status = mz_zip_reader_end(&zip_archive);
    assert(status);
}

winrt::fire_and_forget CodePush::CodePush::DownloadUpdate(JSValueObject updatePackage, bool notifyProgress, ReactPromise<JSValue> promise) noexcept
{
    JSValueObject mutableUpdatePackage = {};
    for (auto& pair : updatePackage)
    {
        mutableUpdatePackage[pair.first] = pair.second.Copy();
    }

    auto downloadUrl{ updatePackage["downloadUrl"].AsString() };
    const uint32_t BufferSize{ 8 * 1024 };

    HttpClient client;
    auto headers{ client.DefaultRequestHeaders() };
    headers.Append(L"Accept-Encoding", L"identity");

    auto storageFolder{ Windows::Storage::ApplicationData::Current().LocalFolder() };
    auto downloadFile{ co_await storageFolder.CreateFileAsync(L"download.zip", Windows::Storage::CreationCollisionOption::ReplaceExisting) };

    HttpRequestMessage reqm{ HttpMethod::Get(), Uri(to_hstring(downloadUrl)) };
    auto resm{ co_await client.SendRequestAsync(reqm, HttpCompletionOption::ResponseHeadersRead) };
    auto totalBytes{ resm.Content().Headers().ContentLength() };
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
        m_context.EmitJSEvent(L"RCTDeviceEventEmitter", L"CodePushDownloadProgress",
            JSValueObject{
                {"totalBytes", totalBytes.GetInt64() },
                {"receivedBytes", receivedBytes }
            });
    }

    inputStream.Close();
    outputStream.Close();
    
    

    if (isZip)
    {
        auto unzippedFolderName{ L"unzipped" };
        auto unzippedFolder = co_await storageFolder.CreateFolderAsync(unzippedFolderName, CreationCollisionOption::ReplaceExisting);
        co_await UnzipAsync(downloadFile, unzippedFolder);

        auto relativeBundlePath{ path(unzippedFolderName) / L"index.windows.bundle" };

        auto metadataFile{ co_await unzippedFolder.GetFileAsync(L"app.json") };
        auto metadataFileStream{ co_await metadataFile.OpenAsync(FileAccessMode::Read) };
        auto metadataFileInputStream{ metadataFileStream.GetInputStreamAt(0) };
        auto buf{ co_await metadataFileInputStream.ReadAsync(Buffer{ 1024 }, 1024, InputStreamOptions::None) };

        //std::stringstream wss;
        //wss.write(buf.data(), buf.Length());

        //co_await metadataFileFromOldUpdate.DeleteAsync();

        mutableUpdatePackage["bundlePath"] = to_string(relativeBundlePath.wstring());

        //bool isSignatureVerificationEnabled = stringPublicKey

        /*
        

            // Merge contents with current update based on the manifest
            String diffManifestFilePath = CodePushUtils.appendPathComponent(unzippedFolderPath,
                    CodePushConstants.DIFF_MANIFEST_FILE_NAME);
            boolean isDiffUpdate = FileUtils.fileAtPathExists(diffManifestFilePath);
            if (isDiffUpdate) {
                String currentPackageFolderPath = getCurrentPackageFolderPath();
                CodePushUpdateUtils.copyNecessaryFilesFromCurrentPackage(diffManifestFilePath, currentPackageFolderPath, newUpdateFolderPath);
                File diffManifestFile = new File(diffManifestFilePath);
                diffManifestFile.delete();
            }

            FileUtils.copyDirectoryContents(unzippedFolderPath, newUpdateFolderPath);
            FileUtils.deleteFileAtPathSilently(unzippedFolderPath);

            // For zip updates, we need to find the relative path to the jsBundle and save it in the
            // metadata so that we can find and run it easily the next time.
            String relativeBundlePath = CodePushUpdateUtils.findJSBundleInUpdateContents(newUpdateFolderPath, expectedBundleFileName);

            if (relativeBundlePath == null) {
                throw new CodePushInvalidUpdateException("Update is invalid - A JS bundle file named \"" + expectedBundleFileName + "\" could not be found within the downloaded contents. Please check that you are releasing your CodePush updates using the exact same JS bundle file name that was shipped with your app's binary.");
            } else {
                if (FileUtils.fileAtPathExists(newUpdateMetadataPath)) {
                    File metadataFileFromOldUpdate = new File(newUpdateMetadataPath);
                    metadataFileFromOldUpdate.delete();
                }

                if (isDiffUpdate) {
                    CodePushUtils.log("Applying diff update.");
                } else {
                    CodePushUtils.log("Applying full update.");
                }

                boolean isSignatureVerificationEnabled = (stringPublicKey != null);

                String signaturePath = CodePushUpdateUtils.getSignatureFilePath(newUpdateFolderPath);
                boolean isSignatureAppearedInBundle = FileUtils.fileAtPathExists(signaturePath);

                if (isSignatureVerificationEnabled) {
                    if (isSignatureAppearedInBundle) {
                        CodePushUpdateUtils.verifyFolderHash(newUpdateFolderPath, newUpdateHash);
                        CodePushUpdateUtils.verifyUpdateSignature(newUpdateFolderPath, newUpdateHash, stringPublicKey);
                    } else {
                        throw new CodePushInvalidUpdateException(
                                "Error! Public key was provided but there is no JWT signature within app bundle to verify. " +
                                "Possible reasons, why that might happen: \n" +
                                "1. You've been released CodePush bundle update using version of CodePush CLI that is not support code signing.\n" +
                                "2. You've been released CodePush bundle update without providing --privateKeyPath option."
                        );
                    }
                } else {
                    if (isSignatureAppearedInBundle) {
                        CodePushUtils.log(
                                "Warning! JWT signature exists in codepush update but code integrity check couldn't be performed because there is no public key configured. " +
                                "Please ensure that public key is properly configured within your application."
                        );
                        CodePushUpdateUtils.verifyFolderHash(newUpdateFolderPath, newUpdateHash);
                    } else {
                        if (isDiffUpdate) {
                            CodePushUpdateUtils.verifyFolderHash(newUpdateFolderPath, newUpdateHash);
                        }
                    }
                }

                CodePushUtils.setJSONValueForKey(updatePackage, CodePushConstants.RELATIVE_BUNDLE_PATH_KEY, relativeBundlePath);
            }

        */
    }
    else
    {
        // Rename the file
        co_await downloadFile.MoveAsync(co_await downloadFile.GetParentAsync(), L"index.windows.bundle", Windows::Storage::NameCollisionOption::ReplaceExisting);
    }

    //promise.Resolve(mutableUpdatePackage.Copy());
    co_return;
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