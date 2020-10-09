#pragma once

#include "NativeModules.h"

#include <string>
#include <filesystem>
#include "winrt/Microsoft.ReactNative.h"
#include "winrt/Windows.Data.Json.h"
#include "winrt/Windows.Foundation.Collections.h"

namespace CodePush
{
	using namespace winrt;
	using namespace Microsoft::ReactNative;
	using namespace Windows::Data::Json;
	using namespace Windows::Foundation::Collections;

	using namespace std;

	struct CodePushConfig
	{
	private:
		inline static const wstring AppVersionConfigKey{ L"appVersion" };
		inline static const wstring BuildVersionConfigKey{ L"buildVersion" };
		inline static const wstring ClientUniqueIDConfigKey{ L"clientUniqueId" };
		inline static const wstring DeploymentKeyConfigKey{ L"deploymentKey" };
		inline static const wstring ServerURLConfigKey{ L"serverUrl" };
		inline static const wstring PublicKeyKey{ L"publicKey" };

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

		static CodePushConfig Init(const ReactContext& context);

	};
}