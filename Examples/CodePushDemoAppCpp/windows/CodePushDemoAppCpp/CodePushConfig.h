#pragma once

#include <string>
#include <filesystem>
#include "winrt/Microsoft.ReactNative.h"
#include "winrt/Windows.Data.Json.h"

namespace CodePush
{
	using namespace winrt;
	using namespace Microsoft::ReactNative;
	using namespace Windows::Data::Json;

	using namespace std;

	struct CodePushConfig
	{
	private:
		static const wstring AppVersionConfigKey{ L"appVersion" };
		static const wstring BuildVersionConfigKey{ L"buildVersion" };
		static const wstring ClientUniqueIDConfigKey{ L"clientUniqueId" };
		static const wstring DeploymentKeyConfigKey{ L"deploymentKey" };
		static const wstring ServerURLConfigKey{ L"serverUrl" };
		static const wstring PublicKeyKey{ L"publicKey" };

		IMap<hstring, hstring> configuration;

	public:
		hstring GetAppVersion() { return configuration.Lookup(AppVersionConfigKey); }
		void SetAppVersion(wstring_view appVersion) { configuration.Insert(AppVersionConfigKey, appVersion); }

		hstring GetBuildVersion() { return configuration.Lookup(BuildVersionConfigKey); }
		
		JsonObject GetConfiguration();

		hstring GetDeploymentKey() { return configuration.Lookup(DeploymentKeyConfigKey); }
		void SetDeploymentKey(wstring_view deploymentKey) { configuration.Insert(DeploymentKeyConfigKey, deploymentKey); }

		hstring GetServerUrl() { return configuration.Lookup(ServerURLConfigKey); }
		void SetServerUrl(wstring_view serverUrl) { configuration.Insert(ServerURLConfigKey, serverUrl); }

		hstring GetPublicKey() { return configuration.Lookup(PublicKeyKey); }
		void SetPublicKey(wstring_view publicKey) { configuration.Insert(PublicKeyKey, publicKey); }

		static CodePushConfig Init(IReactContext const& context);

	};
}