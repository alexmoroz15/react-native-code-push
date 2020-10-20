#pragma once

#include "winrt/Windows.Storage.h"
#include "winrt/Windows.Foundation.h"
#include <filesystem>
#include <string_view>

namespace CodePush
{
	using namespace winrt;
	using namespace Windows::Storage;
	using namespace Windows::Foundation;

	using namespace std;
	using namespace filesystem;

	struct FileUtils
	{
		bool ItemExistsAtPath(const path& itemPath);
		bool FileExistsAtPath(const path& filePath);
		bool FolderExistsAtPath(const path& folderPath);

		IStorageItem GetItemAtPath(const path& itemPath);
		StorageFile GetFileAtPath(const path& filePath);

		static IAsyncOperation<StorageFile> GetFileAtPathAsync(StorageFolder rootFolder, path relPath);
		static IAsyncOperation<StorageFolder> GetFolderAtPathAsync(StorageFolder rootFolder, path relPath);

		static IAsyncOperation<StorageFile> GetOrCreateFileAtPathAsync(StorageFolder rootFolder, path relPath);
		static IAsyncOperation<StorageFolder> GetOrCreateFolderAtPathAsync(StorageFolder rootFolder, path relPath);

		static IAsyncOperation<StorageFile> GetOrCreateFileAsync(StorageFolder rootFolder, wstring_view newFileName);
		static IAsyncOperation<StorageFolder> GetOrCreateFolderAsync(StorageFolder rootFolder, wstring_view newFolderName);

		static IAsyncOperation<StorageFile> CreateFileFromPathAsync(StorageFolder rootFolder, const path& relativePath);
		static IAsyncOperation<hstring> FindFilePathAsync(const StorageFolder& rootFolder, wstring_view fileName);
		static IAsyncAction UnzipAsync(const StorageFile& zipFile, const StorageFolder& destination);
	};
}