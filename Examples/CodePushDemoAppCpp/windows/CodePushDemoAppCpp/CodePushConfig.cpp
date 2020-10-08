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

CodePushConfig CodePushConfig::Init(IReactContext const& context)
{
    CodePushConfig config;

    std::optional<hstring> appVersion;
    std::optional<hstring> buildVersion;
    std::optional<hstring> deploymentKey;
    std::optional<hstring> publicKey;
    std::optional<hstring> serverUrl;

    auto res = context.Properties().Get(ReactPropertyBagHelper::GetName(nullptr, L"Configuration")).try_as<IMap<hstring, hstring>>();
    if (res != nullptr)
    {
        appVersion = res.TryLookup(AppVersionConfigKey);
        buildVersion = res.TryLookup(BuildVersionConfigKey);
        deploymentKey = res.TryLookup(DeploymentKeyConfigKey);
        publicKey = res.TryLookup(PublicKeyKey);
        serverUrl = res.TryLookup(ServerURLConfigKey);
    }

    auto addToConfiguration = [=](hstring key, ) {
        
    };

    /*
    if (appVersion.has_value())
    {
        config.configuration.Insert(AppVersionConfigKey, appVersion.value());
    }

    if (buildVersion.has_value())
    {
        config.buildVersion = buildVersion.value();
    }
    if (clientUniqueId.has_value())
    {
        config.configuration.Insert(ClientUniqueIDConfigKey, clientUniqueId.value());
    }
    if (deploymentKey.has_value())
    {
        config.deploymentKey = deploymentKey.value();
    }
    if (publicKey.has_value())
    {
        config.publicKey = publicKey.value();
    }

    if (serverUrl.has_value())
    {
        config.serverUrl = serverUrl.value();
    }
    else
    {
        config.serverUrl = L"https://codepush.appcenter.ms/";
    }*/
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