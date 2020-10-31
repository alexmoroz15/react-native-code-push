#include "pch.h"

#include "miniz.h"
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Storage.h"
#include "winrt/Windows.Storage.Streams.h"

#include <string>
#include <string_view>
#include <stack>
#include <filesystem>

#include "CodePushNativeModule.h"
#include "FileUtils.h"

using namespace CodePush;

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;

using namespace std;
using namespace filesystem;

/*
IStorageItem GetItemAtPath(const path& itemPath)
{
    if (itemPath.wstring().find(CodePushNativeModule::GetLocalStoragePath().wstring()) != wstring::npos)
    {
        auto rootFolder{ CodePushNativeModule::GetLocalStorageFolder() };
        auto 
    }

    return nullptr;
}
*/

IAsyncOperation<StorageFile> FileUtils::GetFileAtPathAsync(const StorageFolder& rootFolder, wstring_view relPath)
{
    auto folder{ rootFolder };
    size_t start{ 0 };
    size_t end;
    while ((end = relPath.find('\\', start)) != wstring_view::npos)
    {
        hstring pathPart{ relPath.substr(start, end - start) };
        folder = (co_await folder.TryGetItemAsync(pathPart)).try_as<StorageFolder>();
        if (folder == nullptr)
        {
            co_return nullptr;
        }
        start = end + 1;
    }

    auto fileName{ relPath.substr(start) };
    auto file{ (co_await folder.TryGetItemAsync(fileName)).try_as<StorageFile>() };
    co_return file;
}

// Returns the folder at the specified path relative to the rootfolder
IAsyncOperation<StorageFolder> FileUtils::GetFolderAtPathAsync(StorageFolder rootFolder, path relPath)
{
    stack<wstring> pathParts;
    while (relPath.has_parent_path())
    {
        pathParts.push(relPath.stem());
        relPath = relPath.parent_path();
    }

    while (!pathParts.empty())
    {
        auto folder{ (co_await rootFolder.TryGetItemAsync(pathParts.top())).try_as<StorageFolder>() };
        if (folder == nullptr)
        {
            co_return nullptr;
        }
        rootFolder = folder;
        pathParts.pop();
    }

    co_return rootFolder;
}

IAsyncOperation<StorageFile> FileUtils::GetOrCreateFileAtPathAsync(StorageFolder rootFolder, path relPath)
{
    stack<wstring> pathParts;
    while (relPath.has_parent_path())
    {
        pathParts.push(relPath.stem());
        relPath = relPath.parent_path();
    }

    while (pathParts.size() > 2)
    {
        rootFolder = co_await GetOrCreateFolderAsync(rootFolder, pathParts.top());
        pathParts.pop();
    }

    co_return co_await GetOrCreateFileAsync(rootFolder, pathParts.top());
}

IAsyncOperation<StorageFolder> FileUtils::GetOrCreateFolderAtPathAsync(StorageFolder rootFolder, path relPath)
{
    stack<wstring> pathParts;
    while (relPath.has_parent_path())
    {
        pathParts.push(relPath.stem());
        relPath = relPath.parent_path();
    }

    while (!pathParts.empty())
    {
        rootFolder = co_await GetOrCreateFolderAsync(rootFolder, pathParts.top());
        pathParts.pop();
    }

    co_return rootFolder;
}

IAsyncOperation<StorageFile> FileUtils::GetOrCreateFileAsync(StorageFolder rootFolder, wstring_view newFileName)
{
    auto item{ (co_await rootFolder.TryGetItemAsync(newFileName)).try_as<StorageFile>() };
    if (item == nullptr)
    {
        co_return co_await rootFolder.CreateFileAsync(newFileName);
    }
    co_return item;
}

IAsyncOperation<StorageFolder> FileUtils::GetOrCreateFolderAsync(StorageFolder rootFolder, wstring_view newFolderName)
{
    auto item{ (co_await rootFolder.TryGetItemAsync(newFolderName)).try_as<StorageFolder>() };
    if (item == nullptr)
    {
        co_return co_await rootFolder.CreateFolderAsync(newFolderName);
    }
    co_return item;
}

IAsyncOperation<StorageFile> FileUtils::CreateFileFromPathAsync(StorageFolder rootFolder, const path& relativePath)
{
    auto relPath{ relativePath };

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

IAsyncOperation<hstring> FileUtils::FindFilePathAsync(const StorageFolder& rootFolder, wstring_view fileName)
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

IAsyncAction FileUtils::UnzipAsync(const StorageFile& zipFile, const StorageFolder& destination)
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