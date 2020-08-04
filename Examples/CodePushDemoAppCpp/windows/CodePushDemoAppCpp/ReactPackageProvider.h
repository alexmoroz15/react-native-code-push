#pragma once

#include "winrt/Microsoft.ReactNative.h"


namespace winrt::CodePushDemoAppCpp::implementation
{
    struct ReactPackageProvider : winrt::implements<ReactPackageProvider, winrt::Microsoft::ReactNative::IReactPackageProvider>
    {
    public: // IReactPackageProvider
        //ReactPackageProvider();
        ~ReactPackageProvider() {}

        void CreatePackage(winrt::Microsoft::ReactNative::IReactPackageBuilder const &packageBuilder) noexcept;
    };
} // namespace winrt::CodePushDemoAppCpp::implementation


