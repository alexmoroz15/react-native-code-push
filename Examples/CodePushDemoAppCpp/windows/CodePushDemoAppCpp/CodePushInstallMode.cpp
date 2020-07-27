#include "pch.h"
#include "CodePush.h"

namespace CodePush
{
	enum CodePushInstallMode
	{
		Immediate,
		OnNextRestart,
		OnNextResume,
		OnNextSuspend
	};
}