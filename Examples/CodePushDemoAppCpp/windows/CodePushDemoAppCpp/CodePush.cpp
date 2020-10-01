#include "pch.h"

#include "CodePush.h"
#include "CodePushUtils.h"
#include "CodePushPackage.h"
#include "App.h"

#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.Web.Http.h>
#include <winrt/Windows.Web.Http.Headers.h>
#include <winrt/Windows.Storage.Streams.h>

#include <exception>
#include <filesystem>
#include <cstdio>
#include <stack>
#include <string_view>

#include "miniz.h"

#include "AutolinkedNativeModules.g.h"
#include "ReactPackageProvider.h"

using namespace winrt;
using namespace Microsoft::ReactNative;
using namespace Windows::Data::Json;
using namespace Windows::Web::Http;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::Foundation;

using namespace std::filesystem;

void CodePush::CodePush::Initialize(ReactContext const& reactContext) noexcept
{
    m_context = reactContext;
    m_host = g_host;
}