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

const hstring PackageHashKey{ L"packageHash" };

namespace winrt::Microsoft::ReactNative
{
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
}

bool IsPackageBundleLatest(IJsonValue packageMetadata)
{
    // idk
    return false;
}

void CodePush::CodePush::SetJSBundleFile(winrt::hstring latestJSBundleFile)
{
    m_host.InstanceSettings().JavaScriptBundleFile(latestJSBundleFile);
}

hstring CodePush::CodePush::GetJSBundleFileSync()
{
    auto localSettings{ ApplicationData::Current().LocalSettings() };
    auto currentBundleFolderPathBox{ localSettings.Values().TryLookup(L"currentPackageFolderPath") };
    hstring currentBundleFolderPath;
    if (currentBundleFolderPathBox != nullptr)
    {
        currentBundleFolderPath = unbox_value<hstring>(currentBundleFolderPathBox);
        // Would be good to check if the file exists first before defaulting to a main bundle.
        return currentBundleFolderPath;
    }
    return L"";
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
    auto packageFilePath = co_await CodePushPackage::GetCurrentPackageBundlePathAsync(m_assetsBundleFileName);
    if (packageFilePath.empty())
    {
        //There has not been any downloaded updates
        // log stuff
        CodePushUtils::LogBundleUrl(packageFilePath.c_str());
        co_return binaryJSBundleUrl;
    }

    auto packageMetadata = co_await CodePushPackage::GetCurrentPackageAsync();
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

fire_and_forget InitBundle()
{
    auto localStorage{ winrt::Windows::Storage::ApplicationData::Current().LocalFolder() };
    auto currentPackageInfoFile{ (co_await localStorage.TryGetItemAsync(L"codepush.json")).try_as<StorageFile>() };
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

JSValue JsonObject2JSValue(const JsonObject& input)
{
    return nullptr;
}


bool IsRunningBinaryVersion()
{
    return true;
}

winrt::fire_and_forget CodePush::CodePush::GetUpdateMetadata(CodePushUpdateState updateState, ReactPromise<IJsonValue> promise) noexcept
{
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

winrt::fire_and_forget CodePush::CodePush::DownloadUpdate(JSValueObject updatePackage, bool notifyProgress, ReactPromise<JSValue> promise) noexcept
{
    JSValueObject mutableUpdatePackage = {};
    for (auto& pair : updatePackage)
    {
        mutableUpdatePackage[pair.first] = pair.second.Copy();
    }

    auto storageFolder{ Windows::Storage::ApplicationData::Current().LocalFolder() };

    auto newUpdateHash{ updatePackage["packageHash"].AsString() };
    StorageFolder newUpdateFolder{ nullptr };

    auto downloadUrl{ updatePackage["downloadUrl"].AsString() };
    const uint32_t BufferSize{ 8 * 1024 };

    HttpClient client;
    auto headers{ client.DefaultRequestHeaders() };
    headers.Append(L"Accept-Encoding", L"identity");

    auto downloadFile{ co_await storageFolder.CreateFileAsync(L"download.zip", Windows::Storage::CreationCollisionOption::ReplaceExisting) };

    HttpRequestMessage reqm{ HttpMethod::Get(), Uri(to_hstring(downloadUrl)) };
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

        co_await unzippedFolder.RenameAsync(to_hstring(newUpdateHash), NameCollisionOption::ReplaceExisting);
        newUpdateFolder = unzippedFolder;

        mutableUpdatePackage["bundlePath"] = to_string(relativeBundlePath.wstring());
        promise.Resolve(mutableUpdatePackage.Copy());
    }
    else
    {
        // Rename the file
        co_await downloadFile.RenameAsync(L"index.windows.bundle", Windows::Storage::NameCollisionOption::ReplaceExisting);
    }

    auto newUpdateMetadataPath{ path(newUpdateHash) / "CodePush" / "assets" / "app.json" };
    auto newUpdateMetadataFile{ co_await CreateFileFromPathAsync(storageFolder, newUpdateMetadataPath) };

    auto metadataString{ JSValue{ std::move(mutableUpdatePackage) }.ToString() };
    
    // I shouldn't have to do this processing
    size_t i;
    while ((i = metadataString.find("1\n")) != std::string::npos)
    {
        metadataString = metadataString.replace(i, 4, "\"");
    }
    while ((i = metadataString.find("0\n")) != std::string::npos)
    {
        metadataString = metadataString.replace(i, 4, ",\"");
    }
    i = 0;
    while ((i = metadataString.find(": ", i + 2)) != std::string::npos)
    {
        metadataString = metadataString.insert(i, "\"");
    }
    
    co_await FileIO::WriteTextAsync(newUpdateMetadataFile, to_hstring(metadataString));
    co_return;
}

IAsyncAction CodePush::CodePush::InstallPackage(JSValueObject updatePackage)
{
    auto packageHash{ updatePackage["packageHash"].AsString() };

    JsonObject info{};
    auto storageFolder{ Windows::Storage::ApplicationData::Current().LocalFolder() };
    
    StorageFile infoFile{ (co_await storageFolder.TryGetItemAsync(L"codepush.json")).try_as<StorageFile>() };
    if (infoFile != nullptr)
    {
        try
        {
            auto infoText{ co_await FileIO::ReadTextAsync(infoFile) };
            info = JsonObject::Parse(infoText);
        }
        catch (const hresult_error& ex)
        {
            OutputDebugStringW((hstring(L"An unexpected error occurred when reading info file. ") + to_hstring(ex.code()) + L": " + ex.message() + L"\n").c_str());
        }
    }

    auto currentPackageHash{ info.GetNamedString(L"currentPackage", L"") };
    if (!updatePackage["packageHash"].IsNull() && to_hstring(packageHash) == currentPackageHash)
    {
        // The current package is already the one being installed, so we should no-op.
        co_return;
    }

    if (IsPendingUpdate(to_hstring(packageHash)))
    {
        // Delete current package directory
        auto currentPackageFolder{ (co_await storageFolder.TryGetItemAsync(to_hstring(packageHash))).try_as<StorageFolder>() };
        if (currentPackageFolder != nullptr)
        {
            currentPackageFolder.DeleteAsync();
        }
    }
    else
    {
        auto previousPackageHash{ info.GetNamedString(L"previousPackage", L"") };
        if (!previousPackageHash.empty() && to_hstring(packageHash) != previousPackageHash)
        {
            // Delete previous package directory
            auto previousPackageFolder{ (co_await storageFolder.TryGetItemAsync(to_hstring(previousPackageHash))).try_as<StorageFolder>() };
            if (previousPackageFolder != nullptr)
            {
                previousPackageFolder.DeleteAsync();
            }
        }

        if (info.HasKey(L"currentPackage"))
        {
            info.SetNamedValue(L"previousPackage", info.GetNamedValue(L"currentPackage"));
        }
    }

    info.SetNamedValue(L"currentPackage", JsonValue::CreateStringValue(to_hstring(packageHash)));

    if (infoFile == nullptr)
    {
        try
        {
            infoFile = co_await storageFolder.CreateFileAsync(L"codepush.json");
        }
        catch (const hresult_error& ex)
        {
            OutputDebugStringW((hstring(L"An unexpected error occurred when creating package info. ") + to_hstring(ex.code()) + L": " + ex.message() + L"\n").c_str());
        }
    }
    try
    {
        co_await FileIO::WriteTextAsync(infoFile, info.Stringify());

        auto localSettings{ ApplicationData::Current().LocalSettings() };
        auto currentPackageFolder{ co_await storageFolder.GetFolderAsync(to_hstring(packageHash)) };
        auto relPath{ co_await FindFilePathAsync(currentPackageFolder, L"index.windows.bundle") };
        path relPathPath{ std::wstring_view(relPath) };
        path relPathPathStem{ relPathPath.parent_path() };
        localSettings.Values().Insert(L"currentPackageFolderPath", box_value(currentPackageFolder.Path() + L"\\" + relPathPathStem.wstring() + L"\\"));
    }
    catch (const hresult_error& ex)
    {
        OutputDebugStringW((hstring(L"An unexpected error occurred when updating package info. ") + to_hstring(ex.code()) + L": " + ex.message() + L"\n").c_str());
    }
}

fire_and_forget CodePush::CodePush::InstallUpdate(JSValueObject updatePackage, int installMode, int minimumBackgroundDuration, ReactPromise<JSValue> promise) noexcept
{
    //auto asdf{ updatePackage.Copy() };
    co_await InstallPackage(updatePackage.Copy());

    auto pendingHash{ updatePackage["packageHash"].AsString() };
    if (updatePackage["packageHash"].IsNull())
    {
        // Error
    }
    else
    {
        // Save pending update
    }

    promise.Resolve(JSValue::Null);
}

void CodePush::CodePush::IsFailedUpdate(std::wstring packageHash, ReactPromise<bool> promise) noexcept
{
    promise.Resolve(false);
}

void CodePush::CodePush::ClearPendingRestart(ReactPromise<void> promise) noexcept
{
    _restartQueue.clear();
    promise.Resolve();
}