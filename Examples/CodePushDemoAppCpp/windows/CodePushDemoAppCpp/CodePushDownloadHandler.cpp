#include "pch.h"

#include "CodePushDownloadHandler.h"

using namespace CodePush;

CodePushDownloadHandler::CodePushDownloadHandler(
	path downloadFilePath,
	function<void(int64_t, int64_t)> progressCallback,
	function<void(bool)> doneCallback,
	function<void(const hresult_error&)> failCallback)
{

}

void CodePushDownloadHandler::Download(wstring_view url)
{

}