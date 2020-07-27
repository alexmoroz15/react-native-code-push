#include "pch.h"
#include "ReactPackageProvider.h"
#include "NativeModules.h"

#include "CodePush.h"

using namespace winrt::Microsoft::ReactNative;

namespace winrt::CodePushDemoAppCpp::implementation
{

void ReactPackageProvider::CreatePackage(IReactPackageBuilder const &packageBuilder) noexcept
{
    AddAttributedModules(packageBuilder);
}

} // namespace winrt::CodePushDemoAppCpp::implementation


