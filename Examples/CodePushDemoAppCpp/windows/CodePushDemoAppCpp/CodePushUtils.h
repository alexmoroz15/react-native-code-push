#pragma once

#include "NativeModules.h"

namespace CodePush
{
	struct CodePushUtils
	{
		static void Log(std::wstring message);
		static void Log(std::exception error);
	};
}