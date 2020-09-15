#include "pch.h"

#include "App.h"

#include "AutolinkedNativeModules.g.h"
#include "ReactPackageProvider.h"


using namespace winrt::CodePushDemoAppCpp;
using namespace winrt::CodePushDemoAppCpp::implementation;

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

#if BUNDLE
    //InstanceSettings().BundleRootPath(L"ms-appx:///");
    //InstanceSettings().BundleRootPath(L"C:\\GitHub\\react-native-code-push\\Examples\\CodePushDemoAppCpp\\windows\\CodePushDemoAppCpp\\Assets");
    InstanceSettings().BundleRootPath(L"ms-appx:///assets/CodePush/");

    JavaScriptBundleFile(L"index.windows");
    InstanceSettings().UseWebDebugger(false);
    InstanceSettings().UseFastRefresh(false);
#else
    JavaScriptMainModuleName(L"index");
    InstanceSettings().UseWebDebugger(true);
    InstanceSettings().UseFastRefresh(true);
#endif

#if _DEBUG
    InstanceSettings().EnableDeveloperMenu(true);
#else
    InstanceSettings().EnableDeveloperMenu(false);
#endif

    //instanceSettings = InstanceSettings();
    //InstanceSettings().Properties().Set(winrt::Microsoft::ReactNative::ReactPropertyBagHelper::GetName(nullptr, L"MyReactNativeHost"), Host());
    g_host = Host();
    //g_instanceSettings = InstanceSettings();

    RegisterAutolinkedNativeModulePackages(PackageProviders()); // Includes any autolinked modules

    PackageProviders().Append(make<ReactPackageProvider>()); // Includes all modules in this project

    InitializeComponent();
}
