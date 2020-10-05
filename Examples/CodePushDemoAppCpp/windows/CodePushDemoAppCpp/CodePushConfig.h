#pragma once

#include <string>
#include <filesystem>

namespace CodePush
{
	using namespace std;

	struct CodePushConfig
	{
	private:
		wstring appVersion;
		wstring buildVersion;
		wstring configuration;
		wstring deploymentKey;
		wstring serverUrl;
		wstring publicKey;

	public:
		wstring GetAppVersion() { return appVersion; }
		void SetAppVersion(wstring _appVersion) { appVersion = _appVersion; }

		wstring GetBuildVersion() { return buildVersion; }
		
		wstring GetConfiguration() { return configuration; }

		wstring GetDeploymentKey() { return deploymentKey; }
		void SetDeploymentKey(wstring _appVersion) { appVersion = _appVersion; }

		wstring GetServerUrl() { return serverUrl; }
		void SetServerUrl(wstring _serverUrl) { serverUrl = _serverUrl; }

		wstring GetPublicKey() { return publicKey; }
		void SetPublicKey(wstring _publicKey) { publicKey = _publicKey; }

		CodePushConfig Current();
	};
}