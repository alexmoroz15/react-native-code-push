#pragma once

#include "NativeModules.h"

#include <string>
#include <filesystem>
#include <optional>
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

		static CodePushConfig _currentConfig;

	public:
		static CodePushConfig& Current();

		std::optional<hstring> GetAppVersion() { return configuration.TryLookup(AppVersionConfigKey); }
		void SetAppVersion(wstring_view appVersion) { configuration.Insert(AppVersionConfigKey, appVersion); }

		std::optional<hstring> GetBuildVersion() { return configuration.TryLookup(BuildVersionConfigKey); }
		
		JsonObject GetConfiguration();

		std::optional<hstring> GetDeploymentKey() { return configuration.TryLookup(DeploymentKeyConfigKey); }
		void SetDeploymentKey(wstring_view deploymentKey) { configuration.Insert(DeploymentKeyConfigKey, deploymentKey); }

		std::optional<hstring> GetServerUrl() { return configuration.TryLookup(ServerURLConfigKey); }
		void SetServerUrl(wstring_view serverUrl) { configuration.Insert(ServerURLConfigKey, serverUrl); }

		std::optional<hstring> GetPublicKey() { return configuration.TryLookup(PublicKeyKey); }
		void SetPublicKey(wstring_view publicKey) { configuration.Insert(PublicKeyKey, publicKey); }

		static CodePushConfig& Init(const IMap<hstring, hstring>& configMap);

	};
}