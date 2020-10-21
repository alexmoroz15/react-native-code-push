#include "pch.h"

#include "CodePushConfig.h"
#include "winrt/Windows.Foundation.Collections.h"

#include <string_view>

using namespace winrt;
using namespace Windows::Foundation::Collections;

using namespace std;

using namespace CodePush;

CodePushConfig CodePushConfig::_currentConfig{};

CodePushConfig& CodePushConfig::Current()
{
    return _currentConfig;
}

JsonObject CodePushConfig::GetConfiguration()
{
    JsonObject configObject;
    for (const auto& pair : configuration)
    {
        configObject.Insert(pair.Key(), JsonValue::CreateStringValue(pair.Value()));
    }
    return configObject;
}

CodePushConfig& CodePushConfig::Init(const IMap<hstring, hstring>& configMap)
{
    std::optional<hstring> appVersion;
    std::optional<hstring> buildVersion;
    std::optional<hstring> deploymentKey;
    std::optional<hstring> publicKey;
    std::optional<hstring> serverUrl;

    //auto res = context.Properties().Handle().Get(ReactPropertyBagHelper::GetName(nullptr, L"Configuration")).try_as<IMap<hstring, hstring>>();
    if (configMap != nullptr)
    {
        appVersion = configMap.TryLookup(AppVersionConfigKey);
        buildVersion = configMap.TryLookup(BuildVersionConfigKey);
        deploymentKey = configMap.TryLookup(DeploymentKeyConfigKey);
        publicKey = configMap.TryLookup(PublicKeyKey);
        serverUrl = configMap.TryLookup(ServerURLConfigKey);
    }

    _currentConfig.configuration = winrt::single_threaded_map<hstring, hstring>();
    auto addToConfiguration = [=](wstring_view key, std::optional<hstring> optValue) {
        if (optValue.has_value())
        {
            _currentConfig.configuration.Insert(key, optValue.value());
        }
    };

    addToConfiguration(AppVersionConfigKey, appVersion);
    addToConfiguration(BuildVersionConfigKey, buildVersion);
    addToConfiguration(DeploymentKeyConfigKey, deploymentKey);
    addToConfiguration(PublicKeyKey, publicKey);
    addToConfiguration(ServerURLConfigKey, serverUrl);

    // set clientUniqueId

    if (!serverUrl.has_value())
    {
        _currentConfig.configuration.Insert(ServerURLConfigKey, L"https://codepush.appcenter.ms/");
    }
    return _currentConfig;
}

/*
- (instancetype)init
{
    self = [super init];
    NSDictionary *infoDictionary = [[NSBundle mainBundle] infoDictionary];

    NSString *appVersion = [infoDictionary objectForKey:@"CFBundleShortVersionString"];
    NSString *buildVersion = [infoDictionary objectForKey:(NSString *)kCFBundleVersionKey];
    NSString *deploymentKey = [infoDictionary objectForKey:@"CodePushDeploymentKey"];
    NSString *serverURL = [infoDictionary objectForKey:@"CodePushServerURL"];
    NSString *publicKey = [infoDictionary objectForKey:@"CodePushPublicKey"];

    NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
    NSString *clientUniqueId = [userDefaults stringForKey:ClientUniqueIDConfigKey];
    if (clientUniqueId == nil) {
        clientUniqueId = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
        [userDefaults setObject:clientUniqueId forKey:ClientUniqueIDConfigKey];
        [userDefaults synchronize];
    }

    if (!serverURL) {
        serverURL = @"https://codepush.appcenter.ms/";
    }

    _configDictionary = [NSMutableDictionary dictionary];

    if (appVersion) [_configDictionary setObject:appVersion forKey:AppVersionConfigKey];
    if (buildVersion) [_configDictionary setObject:buildVersion forKey:BuildVersionConfigKey];
    if (serverURL) [_configDictionary setObject:serverURL forKey:ServerURLConfigKey];
    if (clientUniqueId) [_configDictionary setObject:clientUniqueId forKey:ClientUniqueIDConfigKey];
    if (deploymentKey) [_configDictionary setObject:deploymentKey forKey:DeploymentKeyConfigKey];
    if (publicKey) [_configDictionary setObject:publicKey forKey:PublicKeyKey];

    return self;
}
*/