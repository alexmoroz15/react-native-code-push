#include "pch.h"

#include "CodePushConfig.h"
#include "winrt/Windows.Foundation.Collections.h"

#include <string_view>

using namespace winrt;
using namespace Windows::Foundation::Collections;

using namespace std;

using namespace CodePush;

JsonObject CodePushConfig::GetConfiguration()
{
    JsonObject configObject;
    for (const auto& pair : configuration)
    {
        configObject.Insert(pair.Key(), JsonValue::CreateStringValue(pair.Value()));
    }
    return configObject;
}

CodePushConfig CodePushConfig::Init(const ReactContext& context)
{
    CodePushConfig config;
    std::optional<hstring> appVersion;
    std::optional<hstring> buildVersion;
    std::optional<hstring> deploymentKey;
    std::optional<hstring> publicKey;
    std::optional<hstring> serverUrl;

    auto res = context.Properties().Handle().Get(ReactPropertyBagHelper::GetName(nullptr, L"Configuration")).try_as<IMap<hstring, hstring>>();
    if (res != nullptr)
    {
        appVersion = res.TryLookup(AppVersionConfigKey);
        buildVersion = res.TryLookup(BuildVersionConfigKey);
        deploymentKey = res.TryLookup(DeploymentKeyConfigKey);
        publicKey = res.TryLookup(PublicKeyKey);
        serverUrl = res.TryLookup(ServerURLConfigKey);
    }

    config.configuration = winrt::single_threaded_map<hstring, hstring>();
    auto addToConfiguration = [=](wstring_view key, std::optional<hstring> optValue) {
        if (optValue.has_value())
        {
            config.configuration.Insert(key, optValue.value());
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
        config.configuration.Insert(ServerURLConfigKey, L"https://codepush.appcenter.ms/");
    }
    return config;
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