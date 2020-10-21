#include "pch.h"

#include "App.h"

#include "AutolinkedNativeModules.g.h"
#include "ReactPackageProvider.h"

#include "winrt/Windows.Storage.h"
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Data.Json.h"

#include <string_view>

#include "CodePushNativeModule.h"
#include "CodePushConfig.h"


using namespace winrt::CodePushDemoAppCpp;
using namespace winrt::CodePushDemoAppCpp::implementation;
using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::ApplicationModel;

using namespace Windows::Storage;
using namespace Windows::Data::Json;

using namespace Microsoft::ReactNative;

using namespace std;

/// <summary>
/// Initializes the singleton application object.  This is the first line of
/// authored code executed, and as such is the logical equivalent of main() or
/// WinMain().
/// </summary>
App::App() noexcept
{
//#if BUNDLE

    //InstanceSettings().BundleRootPath(CodePush::CodePush::GetJSBundleFileSync());

    /*
    auto localSettings{ ApplicationData::Current().LocalSettings() };
    localSettings.Values().Remove(L"currentPackageFolderPath");
    */

    auto localSettings{ ApplicationData::Current().LocalSettings() };
    auto bundleRootPathData{ localSettings.Values().TryLookup(L"bundleRootPath") };
    if (bundleRootPathData != nullptr)
    {
        auto bundleRootPath{ unbox_value<hstring>(bundleRootPathData) };
        InstanceSettings().BundleRootPath(bundleRootPath);
    }


    JavaScriptBundleFile(L"index.windows");
    InstanceSettings().UseWebDebugger(false);
    InstanceSettings().UseFastRefresh(false);
/*#else
    JavaScriptMainModuleName(L"index");
    InstanceSettings().UseWebDebugger(true);
    InstanceSettings().UseFastRefresh(true);
#endif*/

//#if _DEBUG
    InstanceSettings().UseDeveloperSupport(true);
/*#else
    InstanceSettings().UseDeveloperSupport(false);
#endif*/

    RegisterAutolinkedNativeModulePackages(PackageProviders()); // Includes any autolinked modules

    PackageProviders().Append(make<ReactPackageProvider>()); // Includes all modules in this project

    InstanceSettings().Properties().Set(ReactPropertyBagHelper::GetName(nullptr, L"MyReactNativeHost"), Host());

    auto configMap{ winrt::single_threaded_map<hstring, hstring>() };
    configMap.Insert(L"appVersion", L"1.0.0");
    configMap.Insert(L"deploymentKey", L"BJwawsbtm8a1lTuuyN0GPPXMXCO1oUFtA_jJS");
    configMap.Insert(L"serverUrl", L"https://codepush.appcenter.ms/");
    CodePush::CodePushConfig::Init(configMap);

    /*
    auto bundleFile{ co_await CodePush::CodePushNativeModule::GetBundleFileAsync().get() };
    wstring_view bundlePath{ bundleFile.Path() };
    hstring bundleRootPath{ bundlePath.substr(0, bundlePath.rfind('\\')) };
    InstanceSettings().BundleRootPath(bundleRootPath);
    */
    

    
    InitializeComponent();
}

/// <summary>
/// Invoked when the application is launched normally by the end user.  Other entry points
/// will be used such as when the application is launched to open a specific file.
/// </summary>
/// <param name="e">Details about the launch request and process.</param>
void App::OnLaunched(activation::LaunchActivatedEventArgs const& e)
{
    super::OnLaunched(e);

    Frame rootFrame = Window::Current().Content().as<Frame>();
    rootFrame.Navigate(xaml_typename<CodePushDemoAppCpp::MainPage>(), box_value(e.Arguments()));
}

/// <summary>
/// Invoked when application execution is being suspended.  Application state is saved
/// without knowing whether the application will be terminated or resumed with the contents
/// of memory still intact.
/// </summary>
/// <param name="sender">The source of the suspend request.</param>
/// <param name="e">Details about the suspend request.</param>
void App::OnSuspending([[maybe_unused]] IInspectable const& sender, [[maybe_unused]] SuspendingEventArgs const& e)
{
    // Save application state and stop any background activity
}

/// <summary>
/// Invoked when Navigation to a certain page fails
/// </summary>
/// <param name="sender">The Frame which failed navigation</param>
/// <param name="e">Details about the navigation failure</param>
void App::OnNavigationFailed(IInspectable const&, NavigationFailedEventArgs const& e)
{
    throw hresult_error(E_FAIL, hstring(L"Failed to load Page ") + e.SourcePageType().Name);
}
