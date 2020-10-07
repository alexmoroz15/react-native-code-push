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
    
    auto res = reactContext.Properties().Handle().Get(ReactPropertyBagHelper::GetName(nullptr, L"MyReactNativeHost"));
    m_host = res.as<ReactNativeHost>();
    //m_host = g_host;
}

// Helper functions for reading and sending JsonValues to and from JavaScript
namespace winrt::Microsoft::ReactNative
{
    using namespace winrt::Windows::Data::Json;

    void WriteValue(IJSValueWriter const& writer, IJsonValue const& value) noexcept
    {
        switch (value.ValueType())
        {
        case JsonValueType::Object:
            writer.WriteObjectBegin();
            for (const auto& pair : value.GetObject())
            {
                writer.WritePropertyName(pair.Key());
                WriteValue(writer, pair.Value());
            }
            writer.WriteObjectEnd();
            break;
        case JsonValueType::Array:
            writer.WriteArrayBegin();
            for (const auto& elem : value.GetArray())
            {
                WriteValue(writer, elem);
            }
            writer.WriteArrayEnd();
            break;
        case JsonValueType::Boolean:
            writer.WriteBoolean(value.GetBoolean());
            break;
        case JsonValueType::Number:
            writer.WriteDouble(value.GetNumber());
            break;
        case JsonValueType::String:
            writer.WriteString(value.GetString());
            break;
        case JsonValueType::Null:
            writer.WriteNull();
            break;
        }
    }

    void ReadValue(IJSValueReader const& reader, /*out*/ JsonObject& value) noexcept
    {
        if (reader.ValueType() == JSValueType::Object)
        {
            hstring propertyName;
            while (reader.GetNextObjectProperty(propertyName))
            {
                value.Insert(propertyName, ReadValue<IJsonValue>(reader));
            }
        }
    }

    void ReadValue(IJSValueReader const& reader, /*out*/ IJsonValue& value) noexcept
    {
        JsonObject valueObject;
        JsonArray valueArray;
        hstring propertyName;
        switch (reader.ValueType())
        {
        case JSValueType::Object:
            while (reader.GetNextObjectProperty(propertyName))
            {
                valueObject.Insert(propertyName, ReadValue<IJsonValue>(reader));
            }
            value = valueObject;
            break;
        case JSValueType::Array:
            while (reader.GetNextArrayItem())
            {
                valueArray.Append(ReadValue<IJsonValue>(reader));
            }
            value = valueArray;
            break;
        case JSValueType::Boolean:
            value = JsonValue::CreateBooleanValue(reader.GetBoolean());
            break;
        case JSValueType::Double:
            value = JsonValue::CreateNumberValue(reader.GetDouble());
            break;
        case JSValueType::Int64:
            value = JsonValue::CreateNumberValue(static_cast<double>(reader.GetInt64()));
            break;
        case JSValueType::String:
            value = JsonValue::CreateStringValue(reader.GetString());
            break;
        case JSValueType::Null:
            value = JsonValue::CreateNullValue();
            break;
        }
    }
}