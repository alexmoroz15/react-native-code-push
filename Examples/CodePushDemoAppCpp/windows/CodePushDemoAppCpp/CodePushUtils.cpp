#include "pch.h"
#include "CodePushUtils.h"

void CodePush::CodePushUtils::Log(std::wstring message)
{
	OutputDebugStringW((L"[CodePush] " + message + L"\n").c_str());
}

void CodePush::CodePushUtils::Log(std::exception error)
{
	OutputDebugStringA((std::string("[CodePush] Exception") + error.what() + "\n").c_str());
}