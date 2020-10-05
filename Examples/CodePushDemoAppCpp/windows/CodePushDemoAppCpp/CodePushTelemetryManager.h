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
	public:
		static JsonObject GetBinaryUpdateReport(wstring appVersion);
		static JsonObject GetRetryStatusReport();
		static JsonObject GetRollbackReport(JsonObject lastFailedPackage);
		static JsonObject GetUpdateReport(JsonObject currentPackage);
		static void RecordStatusReported(JsonObject statusReport);
		static void SaveStatusReportForRetry(JsonObject statusReport);
	};
}