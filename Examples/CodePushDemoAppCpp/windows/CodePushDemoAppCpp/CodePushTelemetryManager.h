#pragma once

#include "winrt/Windows.Data.Json.h"
#include <string>
#include <filesystem>

/*
@interface CodePushTelemetryManager : NSObject

+ (NSDictionary *)getBinaryUpdateReport:(NSString *)appVersion;
+ (NSDictionary *)getRetryStatusReport;
+ (NSDictionary *)getRollbackReport:(NSDictionary *)lastFailedPackage;
+ (NSDictionary *)getUpdateReport:(NSDictionary *)currentPackage;
+ (void)recordStatusReported:(NSDictionary *)statusReport;
+ (void)saveStatusReportForRetry:(NSDictionary *)statusReport;

@end
*/

namespace CodePush
{
	using namespace winrt;
	using namespace Windows::Data::Json;
	
	using namespace std;

	struct CodePushTelemetryManager
	{
	private:
		static void ClearRetryStatusReport();
		static wstring_view GetDeploymentKeyFromStatusReportIdentifier(wstring_view statusReportIdentifier);
		static hstring GetPackageStatusReportIdentifier(const JsonObject& package);
		static hstring GetPreviousStatusReportIdentifier();
		static wstring_view GetVersionLabelFromStatusReportIdentifier(wstring_view statusReportIdentifier);
		static bool IsStatusReportIdentifierCodePushLabel(wstring_view statusReportIdentifier);
		static void SaveStatusReportedForIdentifier(wstring_view appVersionOrPackageIdentifier);
	public:
		static JsonObject GetBinaryUpdateReport(wstring_view appVersion);
		static JsonObject GetRetryStatusReport();
		static JsonObject GetRollbackReport(const JsonObject& lastFailedPackage);
		static JsonObject GetUpdateReport(const JsonObject& currentPackage);
		static void RecordStatusReported(const JsonObject& statusReport);
		static void SaveStatusReportForRetry(const JsonObject& statusReport);
	};
}