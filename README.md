# React Native for Windows Module for CodePush

> ⚠ This repo contains the **[React Native for Windows](https://aka.ms/reactnative)** implementation of the CodePush module

> ⚠ This module should only be used in React Native for Windows apps. Visit the main [react-native-code-push repository](https://github.com/microsoft/react-native-code-push) if you're building apps that target iOS and Android.

This plugin provides client-side integration for the [CodePush service](https://microsoft.github.io/code-push/), allowing you to easily add a dynamic update experience to your React Native for Windows apps.

## How does it work?

A React Native app is composed of JavaScript files and any accompanying [images](https://facebook.github.io/react-native/docs/images.html#content), which are bundled together by the [packager](https://github.com/facebook/react-native/tree/master/packager) and distributed as part of a platform-specific binary (i.e. an `.ipa` or `.apk` file). Once the app is released, updating either the JavaScript code (e.g. making bug fixes, adding new features) or image assets, requires you to recompile and redistribute the entire binary, which of course, includes any review time associated with the store(s) you are publishing to.

The CodePush plugin helps get product improvements in front of your end users instantly, by keeping your JavaScript and images synchronized with updates you release to the CodePush server. This way, your app gets the benefits of an offline mobile experience, as well as the "web-like" agility of side-loading updates as soon as they are available. It's a win-win!

In order to ensure that your end users always have a functioning version of your app, the CodePush plugin maintains a copy of the previous update, so that in the event that you accidentally push an update which includes a crash, it can automatically roll back. This way, you can rest assured that your newfound release agility won't result in users becoming blocked before you have a chance to [roll back](https://docs.microsoft.com/en-us/appcenter/distribution/codepush/cli#rolling-back-updates) on the server. It's a win-win-win!

*Note: Any product changes which touch native code cannot be distributed via CodePush, and therefore, must be updated via the appropriate store(s).*

## Getting Started

For now, all files are located in Examples/CodePushDemoAppCpp/windows/CodePushDemoAppCpp

To install in your native app, copy over and include in your windows project the following files:
- All source and headers files that begin with CodePush
- FileUtils.h
- FileUtils.cpp
- miniz.c
- miniz.h

Disable precompiled headers for miniz.c

Add the following code to App.cpp:

    InstanceSettings().Properties().Set(winrt::Microsoft::ReactNative::ReactPropertyBagHelper::GetName(nullptr, L"MyReactNativeHost"), Host());
    
    auto configMap{ winrt::single_threaded_map<hstring, hstring>() };
    configMap.Insert(L"appVersion", L"1.0.0");
    configMap.Insert(L"deploymentKey", L"<Your deployment key here>");
    CodePush::CodePushConfig::Init(configMap);
    
    CodePush::CodePushNativeModule::GetBundleFileAsync();

## Using the Module

TODO

## API Reference

TODO

---


This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.