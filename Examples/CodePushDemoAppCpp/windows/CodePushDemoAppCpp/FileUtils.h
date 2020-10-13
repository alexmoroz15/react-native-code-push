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

		IAsyncOperation<StorageFile> GetOrCreateFileAsync(StorageFolder rootFolder, wstring_view newFileName);
		IAsyncOperation<StorageFolder> GetOrCreateFolderAsync(StorageFolder rootFolder, wstring_view newFolderName);
		IAsyncOperation<StorageFolder> GetFolderAtPath(StorageFolder rootFolder, const path& relPath);

		IAsyncOperation<StorageFile> CreateFileFromPathAsync(StorageFolder rootFolder, const path& relativePath);
		IAsyncOperation<hstring> FindFilePathAsync(const StorageFolder& rootFolder, wstring_view fileName);
		IAsyncAction UnzipAsync(const StorageFile& zipFile, const StorageFolder& destination);
	};
}