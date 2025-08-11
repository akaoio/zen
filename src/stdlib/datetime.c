/*
 * datetime.c
 * Date and time functions for ZEN stdlib
 *
 * Complete implementation with parsing, formatting, arithmetic,
 * timezone handling, and UTC conversion capabilities.
 */

#define _GNU_SOURCE  // For strptime
#include "zen/core/error.h"
#include "zen/core/memory.h"
#include "zen/core/runtime_value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Helper function to create datetime object from struct tm and timestamp
static RuntimeValue *datetime_create_datetime_object(const struct tm *timeinfo, time_t timestamp)
{
    RuntimeValue *result = rv_new_object();
    if (!result || !timeinfo) {
        return result;
    }

    rv_object_set(result, "year", rv_new_number(timeinfo->tm_year + 1900));
    rv_object_set(result, "month", rv_new_number(timeinfo->tm_mon + 1));
    rv_object_set(result, "day", rv_new_number(timeinfo->tm_mday));
    rv_object_set(result, "hour", rv_new_number(timeinfo->tm_hour));
    rv_object_set(result, "minute", rv_new_number(timeinfo->tm_min));
    rv_object_set(result, "second", rv_new_number(timeinfo->tm_sec));
    rv_object_set(result, "weekday", rv_new_number(timeinfo->tm_wday));
    rv_object_set(result, "yearday", rv_new_number(timeinfo->tm_yday));
    rv_object_set(result, "timestamp", rv_new_number((double)timestamp));

    return result;
}

// Complete datetime functions with full POSIX time support

RuntimeValue *datetime_now(RuntimeValue **args, size_t argc)
{
    (void)args;
    (void)argc;

    // Get current time
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    if (!timeinfo) {
        return rv_new_error("Failed to get current time", -1);
    }

    // Create datetime object with year, month, day, hour, minute, second
    RuntimeValue *dt_obj = rv_new_object();
    if (!dt_obj) {
        return rv_new_error("Failed to create datetime object", -1);
    }

    // Add year
    RuntimeValue *year = rv_new_number(timeinfo->tm_year + 1900);
    rv_object_set(dt_obj, "year", year);
    rv_unref(year);

    // Add month (1-12 instead of 0-11)
    RuntimeValue *month = rv_new_number(timeinfo->tm_mon + 1);
    rv_object_set(dt_obj, "month", month);
    rv_unref(month);

    // Add day
    RuntimeValue *day = rv_new_number(timeinfo->tm_mday);
    rv_object_set(dt_obj, "day", day);
    rv_unref(day);

    // Add hour
    RuntimeValue *hour = rv_new_number(timeinfo->tm_hour);
    rv_object_set(dt_obj, "hour", hour);
    rv_unref(hour);

    // Add minute
    RuntimeValue *minute = rv_new_number(timeinfo->tm_min);
    rv_object_set(dt_obj, "minute", minute);
    rv_unref(minute);

    // Add second
    RuntimeValue *second = rv_new_number(timeinfo->tm_sec);
    rv_object_set(dt_obj, "second", second);
    rv_unref(second);

    // Add weekday (0=Sunday, 1=Monday, etc.)
    RuntimeValue *weekday = rv_new_number(timeinfo->tm_wday);
    rv_object_set(dt_obj, "weekday", weekday);
    rv_unref(weekday);

    // Add timestamp (seconds since epoch)
    RuntimeValue *timestamp = rv_new_number((double)rawtime);
    rv_object_set(dt_obj, "timestamp", timestamp);
    rv_unref(timestamp);

    return dt_obj;
}

RuntimeValue *datetime_format(RuntimeValue **args, size_t argc)
{
    if (argc < 1 || argc > 2) {
        return rv_new_error("formatDate requires 1 or 2 arguments (timestamp, [format])", -1);
    }

    if (!args[0] || args[0]->type != RV_NUMBER) {
        return rv_new_error("formatDate requires a numeric timestamp", -1);
    }

    time_t timestamp = (time_t)args[0]->data.number;
    const char *format = "%Y-%m-%d %H:%M:%S";  // Default format

    // If format string provided, use it
    if (argc == 2) {
        if (!args[1] || args[1]->type != RV_STRING) {
            return rv_new_error("formatDate format must be a string", -1);
        }
        format = args[1]->data.string.data;
    }

    struct tm *timeinfo = localtime(&timestamp);
    if (!timeinfo) {
        return rv_new_error("Invalid timestamp", -1);
    }

    char buffer[256];
    size_t result = strftime(buffer, sizeof(buffer), format, timeinfo);
    if (result == 0) {
        return rv_new_error("Failed to format date", -1);
    }

    return rv_new_string(buffer);
}

RuntimeValue *datetime_parse(RuntimeValue **args, size_t argc)
{
    if (argc < 1 || argc > 2) {
        return rv_new_error("parseDate requires 1 or 2 arguments (date_string, [format])", -1);
    }

    if (!args[0] || args[0]->type != RV_STRING) {
        return rv_new_error("parseDate requires a string date", -1);
    }

    const char *date_str = args[0]->data.string.data;
    const char *format = "%Y-%m-%d %H:%M:%S";  // Default format

    if (argc == 2) {
        if (!args[1] || args[1]->type != RV_STRING) {
            return rv_new_error("parseDate format must be a string", -1);
        }
        format = args[1]->data.string.data;
    }

    struct tm timeinfo = {0};
    char *result = strptime(date_str, format, &timeinfo);
    if (!result) {
        return rv_new_error("Failed to parse date string", -1);
    }

    time_t timestamp = mktime(&timeinfo);
    if (timestamp == -1) {
        return rv_new_error("Invalid date/time", -1);
    }

    return rv_new_number((double)timestamp);
}

RuntimeValue *datetime_add(RuntimeValue **args, size_t argc)
{
    if (argc != 3) {
        return rv_new_error("datetime_add() requires 3 arguments: datetime, value, unit", -1);
    }

    if (!args[0] || args[0]->type != RV_OBJECT) {
        return rv_new_error("datetime_add() requires datetime object as first argument", -1);
    }

    if (!args[1] || args[1]->type != RV_NUMBER) {
        return rv_new_error("datetime_add() requires number as second argument", -1);
    }

    if (!args[2] || args[2]->type != RV_STRING) {
        return rv_new_error("datetime_add() requires unit string as third argument", -1);
    }

    RuntimeValue *datetime_obj = args[0];
    double value = args[1]->data.number;
    const char *unit = args[2]->data.string.data;

    // Get timestamp from datetime object
    RuntimeValue *timestamp_rv = rv_object_get(datetime_obj, "timestamp");
    if (!timestamp_rv || timestamp_rv->type != RV_NUMBER) {
        return rv_new_error("datetime_add() requires datetime object with timestamp field", -1);
    }

    time_t timestamp = (time_t)timestamp_rv->data.number;

    // Add based on unit
    if (strcmp(unit, "seconds") == 0) {
        timestamp += (time_t)value;
    } else if (strcmp(unit, "minutes") == 0) {
        timestamp += (time_t)(value * 60);
    } else if (strcmp(unit, "hours") == 0) {
        timestamp += (time_t)(value * 3600);
    } else if (strcmp(unit, "days") == 0) {
        timestamp += (time_t)(value * 86400);
    } else {
        return rv_new_error("datetime_add() unsupported unit (use: seconds, minutes, hours, days)",
                            -1);
    }

    // Create new datetime object from modified timestamp
    struct tm *timeinfo = localtime(&timestamp);
    if (!timeinfo) {
        return rv_new_error("datetime_add() failed to convert timestamp", -1);
    }

    return datetime_create_datetime_object(timeinfo, timestamp);
}

RuntimeValue *datetime_subtract(RuntimeValue **args, size_t argc)
{
    if (argc != 3) {
        return rv_new_error("datetime_subtract() requires 3 arguments: datetime, value, unit", -1);
    }

    if (!args[0] || args[0]->type != RV_OBJECT) {
        return rv_new_error("datetime_subtract() requires datetime object as first argument", -1);
    }

    if (!args[1] || args[1]->type != RV_NUMBER) {
        return rv_new_error("datetime_subtract() requires number as second argument", -1);
    }

    if (!args[2] || args[2]->type != RV_STRING) {
        return rv_new_error("datetime_subtract() requires unit string as third argument", -1);
    }

    RuntimeValue *datetime_obj = args[0];
    double value = args[1]->data.number;
    const char *unit = args[2]->data.string.data;

    // Get timestamp from datetime object
    RuntimeValue *timestamp_rv = rv_object_get(datetime_obj, "timestamp");
    if (!timestamp_rv || timestamp_rv->type != RV_NUMBER) {
        return rv_new_error("datetime_subtract() requires datetime object with timestamp field",
                            -1);
    }

    time_t timestamp = (time_t)timestamp_rv->data.number;

    // Subtract based on unit
    if (strcmp(unit, "seconds") == 0) {
        timestamp -= (time_t)value;
    } else if (strcmp(unit, "minutes") == 0) {
        timestamp -= (time_t)(value * 60);
    } else if (strcmp(unit, "hours") == 0) {
        timestamp -= (time_t)(value * 3600);
    } else if (strcmp(unit, "days") == 0) {
        timestamp -= (time_t)(value * 86400);
    } else {
        return rv_new_error(
            "datetime_subtract() unsupported unit (use: seconds, minutes, hours, days)", -1);
    }

    // Create new datetime object from modified timestamp
    struct tm *timeinfo = localtime(&timestamp);
    if (!timeinfo) {
        return rv_new_error("datetime_subtract() failed to convert timestamp", -1);
    }

    return datetime_create_datetime_object(timeinfo, timestamp);
}

RuntimeValue *datetime_diff(RuntimeValue **args, size_t argc)
{
    if (argc < 2 || argc > 3) {
        return rv_new_error(
            "datetime_diff() requires 2 or 3 arguments: datetime1, datetime2, [unit]", -1);
    }

    if (!args[0] || args[0]->type != RV_OBJECT) {
        return rv_new_error("datetime_diff() requires datetime object as first argument", -1);
    }

    if (!args[1] || args[1]->type != RV_OBJECT) {
        return rv_new_error("datetime_diff() requires datetime object as second argument", -1);
    }

    // Get timestamps from both datetime objects
    RuntimeValue *timestamp1_rv = rv_object_get(args[0], "timestamp");
    RuntimeValue *timestamp2_rv = rv_object_get(args[1], "timestamp");

    if (!timestamp1_rv || timestamp1_rv->type != RV_NUMBER) {
        return rv_new_error("datetime_diff() first datetime missing timestamp field", -1);
    }

    if (!timestamp2_rv || timestamp2_rv->type != RV_NUMBER) {
        return rv_new_error("datetime_diff() second datetime missing timestamp field", -1);
    }

    double timestamp1 = timestamp1_rv->data.number;
    double timestamp2 = timestamp2_rv->data.number;
    double diff_seconds = timestamp1 - timestamp2;

    // Default unit is seconds, but can be specified
    const char *unit = "seconds";
    if (argc == 3) {
        if (!args[2] || args[2]->type != RV_STRING) {
            return rv_new_error("datetime_diff() third argument must be unit string", -1);
        }
        unit = args[2]->data.string.data;
    }

    // Convert difference to requested unit
    double result;
    if (strcmp(unit, "seconds") == 0) {
        result = diff_seconds;
    } else if (strcmp(unit, "minutes") == 0) {
        result = diff_seconds / 60.0;
    } else if (strcmp(unit, "hours") == 0) {
        result = diff_seconds / 3600.0;
    } else if (strcmp(unit, "days") == 0) {
        result = diff_seconds / 86400.0;
    } else {
        return rv_new_error("datetime_diff() unsupported unit (use: seconds, minutes, hours, days)",
                            -1);
    }

    return rv_new_number(result);
}

RuntimeValue *datetime_timezone(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("datetime_timezone() requires 1 argument: datetime", -1);
    }

    if (!args[0] || args[0]->type != RV_OBJECT) {
        return rv_new_error("datetime_timezone() requires datetime object", -1);
    }

    RuntimeValue *timestamp_rv = rv_object_get(args[0], "timestamp");
    if (!timestamp_rv || timestamp_rv->type != RV_NUMBER) {
        return rv_new_error("datetime_timezone() datetime missing timestamp field", -1);
    }

    time_t timestamp = (time_t)timestamp_rv->data.number;
    struct tm *timeinfo = localtime(&timestamp);

    if (!timeinfo) {
        return rv_new_error("datetime_timezone() failed to get timezone info", -1);
    }

    // Get timezone offset in seconds from UTC
    long timezone_offset = 0;

#ifdef __linux__
    timezone_offset = -timeinfo->tm_gmtoff;
#else
    // Fallback for systems without tm_gmtoff
    time_t utc_time = mktime(gmtime(&timestamp));
    time_t local_time = mktime(timeinfo);
    timezone_offset = (long)(utc_time - local_time);
#endif

    // Return timezone info as object
    RuntimeValue *result = rv_new_object();
    rv_object_set(result, "offset_seconds", rv_new_number((double)timezone_offset));
    rv_object_set(result, "offset_hours", rv_new_number((double)timezone_offset / 3600.0));
    rv_object_set(result, "name", rv_new_string(timeinfo->tm_zone ? timeinfo->tm_zone : "Unknown"));
    rv_object_set(result, "is_dst", rv_new_boolean(timeinfo->tm_isdst > 0));

    return result;
}

RuntimeValue *datetime_utc(RuntimeValue **args, size_t argc)
{
    if (argc != 1) {
        return rv_new_error("datetime_utc() requires 1 argument: datetime", -1);
    }

    if (!args[0] || args[0]->type != RV_OBJECT) {
        return rv_new_error("datetime_utc() requires datetime object", -1);
    }

    RuntimeValue *timestamp_rv = rv_object_get(args[0], "timestamp");
    if (!timestamp_rv || timestamp_rv->type != RV_NUMBER) {
        return rv_new_error("datetime_utc() datetime missing timestamp field", -1);
    }

    time_t timestamp = (time_t)timestamp_rv->data.number;
    struct tm *utc_timeinfo = gmtime(&timestamp);

    if (!utc_timeinfo) {
        return rv_new_error("datetime_utc() failed to convert to UTC", -1);
    }

    // Create UTC datetime object
    RuntimeValue *result = datetime_create_datetime_object(utc_timeinfo, timestamp);
    if (result) {
        rv_object_set(result, "is_utc", rv_new_boolean(true));
    }

    return result;
}