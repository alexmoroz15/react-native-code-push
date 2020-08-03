#pragma once

#include "App.xaml.g.h"

//extern winrt::Microsoft::ReactNative::ReactInstanceSettings g_instanceSettings;
//extern winrt::Microsoft::ReactNative::IReactContext g_reactContext;
extern winrt::Microsoft::ReactNative::ReactNativeHost g_host;

namespace winrt::CodePushDemoAppCpp::implementation
{
    struct App : AppT<App>
    {
        App() noexcept;
    };
} // namespace winrt::CodePushDemoAppCpp::implementation


