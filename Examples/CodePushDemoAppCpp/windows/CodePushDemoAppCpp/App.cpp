#include "pch.h"

#include "App.h"

#include "AutolinkedNativeModules.g.h"
#include "ReactPackageProvider.h"

#include "winrt/Windows.Storage.h"
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Data.Json.h"

using namespace winrt::CodePushDemoAppCpp;
using namespace winrt::CodePushDemoAppCpp::implementation;

using namespace winrt;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Data::Json;

//winrt::Microsoft::ReactNative::ReactInstanceSettings g_instanceSettings{ nullptr };
//winrt::Microsoft::ReactNative::IReactContext g_reactContext{ nullptr };
winrt::Microsoft::ReactNative::ReactNativeHost g_host{ nullptr };

/// <summary>
/// Initializes the singleton application object.  This is the first line of
/// authored code executed, and as such is the logical equivalent of main() or
/// WinMain().
/// </summary>
App::App() noexcept
{
    MainComponentName(L"CodePushDemoAppCpp");

//#if BUNDLE
    
    auto localSettings{ ApplicationData::Current().LocalSettings() };
    auto currentBundleFolderPathBox{ localSettings.Values().TryLookup(L"currentPackageFolderPath") };
    hstring currentBundleFolderPath;
    if (currentBundleFolderPathBox != nullptr)
    {
        currentBundleFolderPath = unbox_value<hstring>(currentBundleFolderPathBox);
        // Would be good to check if the file exists first before defaulting to a main bundle.
        InstanceSettings().BundleRootPath(currentBundleFolderPath);
    }

    JavaScriptBundleFile(L"index.windows");
    InstanceSettings().UseWebDebugger(false);
    InstanceSettings().UseFastRefresh(false);
/*#else
    JavaScriptMainModuleName(L"index");
    InstanceSettings().UseWebDebugger(true);
    InstanceSettings().UseFastRefresh(true);
#endif*/

/*#if _DEBUG
    InstanceSettings().EnableDeveloperMenu(true);
#else*/
    InstanceSettings().EnableDeveloperMenu(false);
//#endif

    //instanceSettings = InstanceSettings();
    //InstanceSettings().Properties().Set(winrt::Microsoft::ReactNative::ReactPropertyBagHelper::GetName(nullptr, L"MyReactNativeHost"), Host());
    g_host = Host();
    //g_instanceSettings = InstanceSettings();

    RegisterAutolinkedNativeModulePackages(PackageProviders()); // Includes any autolinked modules

    PackageProviders().Append(make<ReactPackageProvider>()); // Includes all modules in this project

    InitializeComponent();
}
