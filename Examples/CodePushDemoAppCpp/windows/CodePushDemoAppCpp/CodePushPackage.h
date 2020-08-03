#pragma once

#include <winrt/Windows.Data.Json.h>

namespace CodePush
{
	struct CodePushPackage
	{
	private:
	public:
		static winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Data::Json::JsonValue> GetCurrentPackage();
	};
}