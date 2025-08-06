/*
 * datetime.c
 * Date and Time Functions for ZEN stdlib  
 * 
 * Provides comprehensive date/time operations including parsing, formatting,
 * timezone handling, and duration calculations
 */

#define _GNU_SOURCE
#include "zen/types/value.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <stdio.h>

// Constants for datetime calculations
#define SECONDS_PER_MINUTE 60
#define SECONDS_PER_HOUR   3600
#define SECONDS_PER_DAY    86400
#define DAYS_PER_WEEK      7

/**
 * @brief Get current Unix timestamp
 * @return Current timestamp as number value
 */
Value* zen_datetime_now(void) {
    time_t now = time(NULL);
    return value_new_number((double)now);
}

/**
 * @brief Get current timestamp in milliseconds
 * @return Current timestamp in milliseconds as number value
 */
Value* zen_datetime_now_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
        double ms = (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
        return value_new_number(ms);
    } else {
        // Fallback to seconds if clock_gettime fails
        time_t now = time(NULL);
        return value_new_number((double)now * 1000.0);
    }
}

/**
 * @brief Format timestamp to string using format specifier
 * @param timestamp_value Timestamp to format (unix timestamp)
 * @param format_value Format string (strftime compatible)
 * @return Formatted date string
 */
Value* zen_datetime_format(const Value* timestamp_value, const Value* format_value) {
    if (!timestamp_value || timestamp_value->type != VALUE_NUMBER) {
        return value_new_string("");
    }
    
    const char* format = "%Y-%m-%d %H:%M:%S"; // Default format
    if (format_value && format_value->type == VALUE_STRING) {
        format = format_value->as.string;
    }
    
    time_t timestamp = (time_t)timestamp_value->as.number;
    struct tm* timeinfo = localtime(&timestamp);
    
    if (!timeinfo) {
        return value_new_string("");
    }
    
    char buffer[256];
    size_t result = strftime(buffer, sizeof(buffer), format, timeinfo);
    
    if (result == 0) {
        return value_new_string("");
    }
    
    return value_new_string(buffer);
}

/**
 * @brief Parse date string to timestamp
 * @param date_string_value Date string to parse
 * @param format_value Format specifier (strptime compatible)
 * @return Unix timestamp as number value, or error if parsing fails
 */
Value* zen_datetime_parse(const Value* date_string_value, const Value* format_value) {
    if (!date_string_value || date_string_value->type != VALUE_STRING) {
        Value* error = value_new(VALUE_ERROR);
        if (error && error->as.error) {
            error->as.error->message = strdup("Invalid date string");
            error->as.error->code = -1;
        }
        return error;
    }
    
    const char* format = "%Y-%m-%d %H:%M:%S"; // Default format
    if (format_value && format_value->type == VALUE_STRING) {
        format = format_value->as.string;
    }
    
    struct tm timeinfo = {0};
    char* result = strptime(date_string_value->as.string, format, &timeinfo);
    
    if (!result) {
        Value* error = value_new(VALUE_ERROR);
        if (error && error->as.error) {
            error->as.error->message = strdup("Failed to parse date string");
            error->as.error->code = -1;
        }
        return error;
    }
    
    time_t timestamp = mktime(&timeinfo);
    if (timestamp == -1) {
        Value* error = value_new(VALUE_ERROR);
        if (error && error->as.error) {
            error->as.error->message = strdup("Invalid date/time values");
            error->as.error->code = -1;
        }
        return error;
    }
    
    return value_new_number((double)timestamp);
}

/**
 * @brief Add duration to timestamp
 * @param timestamp_value Base timestamp
 * @param duration_value Duration in seconds to add
 * @return New timestamp with added duration
 */
Value* zen_datetime_add(const Value* timestamp_value, const Value* duration_value) {
    if (!timestamp_value || timestamp_value->type != VALUE_NUMBER ||
        !duration_value || duration_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    double timestamp = timestamp_value->as.number;
    double duration = duration_value->as.number;
    
    return value_new_number(timestamp + duration);
}

/**
 * @brief Subtract duration from timestamp
 * @param timestamp_value Base timestamp
 * @param duration_value Duration in seconds to subtract
 * @return New timestamp with subtracted duration
 */
Value* zen_datetime_subtract(const Value* timestamp_value, const Value* duration_value) {
    if (!timestamp_value || timestamp_value->type != VALUE_NUMBER ||
        !duration_value || duration_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    double timestamp = timestamp_value->as.number;
    double duration = duration_value->as.number;
    
    return value_new_number(timestamp - duration);
}

/**
 * @brief Calculate difference between two timestamps
 * @param timestamp1_value First timestamp
 * @param timestamp2_value Second timestamp
 * @return Difference in seconds (timestamp1 - timestamp2)
 */
Value* zen_datetime_diff(const Value* timestamp1_value, const Value* timestamp2_value) {
    if (!timestamp1_value || timestamp1_value->type != VALUE_NUMBER ||
        !timestamp2_value || timestamp2_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    double timestamp1 = timestamp1_value->as.number;
    double timestamp2 = timestamp2_value->as.number;
    
    return value_new_number(timestamp1 - timestamp2);
}

/**
 * @brief Get year from timestamp
 * @param timestamp_value Timestamp to extract year from
 * @return Year as number value
 */
Value* zen_datetime_get_year(const Value* timestamp_value) {
    if (!timestamp_value || timestamp_value->type != VALUE_NUMBER) {
        return value_new_number(1970);
    }
    
    time_t timestamp = (time_t)timestamp_value->as.number;
    struct tm* timeinfo = localtime(&timestamp);
    
    if (!timeinfo) {
        return value_new_number(1970);
    }
    
    return value_new_number((double)(timeinfo->tm_year + 1900));
}

/**
 * @brief Get month from timestamp (1-12)
 * @param timestamp_value Timestamp to extract month from
 * @return Month as number value (1-12)
 */
Value* zen_datetime_get_month(const Value* timestamp_value) {
    if (!timestamp_value || timestamp_value->type != VALUE_NUMBER) {
        return value_new_number(1);
    }
    
    time_t timestamp = (time_t)timestamp_value->as.number;
    struct tm* timeinfo = localtime(&timestamp);
    
    if (!timeinfo) {
        return value_new_number(1);
    }
    
    return value_new_number((double)(timeinfo->tm_mon + 1));
}

/**
 * @brief Get day of month from timestamp (1-31)
 * @param timestamp_value Timestamp to extract day from
 * @return Day as number value (1-31)
 */
Value* zen_datetime_get_day(const Value* timestamp_value) {
    if (!timestamp_value || timestamp_value->type != VALUE_NUMBER) {
        return value_new_number(1);
    }
    
    time_t timestamp = (time_t)timestamp_value->as.number;
    struct tm* timeinfo = localtime(&timestamp);
    
    if (!timeinfo) {
        return value_new_number(1);
    }
    
    return value_new_number((double)timeinfo->tm_mday);
}

/**
 * @brief Get hour from timestamp (0-23)
 * @param timestamp_value Timestamp to extract hour from
 * @return Hour as number value (0-23)
 */
Value* zen_datetime_get_hour(const Value* timestamp_value) {
    if (!timestamp_value || timestamp_value->type != VALUE_NUMBER) {
        return value_new_number(0);
    }
    
    time_t timestamp = (time_t)timestamp_value->as.number;
    struct tm* timeinfo = localtime(&timestamp);
    
    if (!timeinfo) {
        return value_new_number(0);
    }
    
    return value_new_number((double)timeinfo->tm_hour);
}

/**
 * @brief Get minute from timestamp (0-59)
 * @param timestamp_value Timestamp to extract minute from
 * @return Minute as number value (0-59)
 */
Value* zen_datetime_get_minute(const Value* timestamp_value) {
    if (!timestamp_value || timestamp_value->type != VALUE_NUMBER) {
        return value_new_number(0);
    }
    
    time_t timestamp = (time_t)timestamp_value->as.number;
    struct tm* timeinfo = localtime(&timestamp);
    
    if (!timeinfo) {
        return value_new_number(0);
    }
    
    return value_new_number((double)timeinfo->tm_min);
}

/**
 * @brief Get second from timestamp (0-59)
 * @param timestamp_value Timestamp to extract second from
 * @return Second as number value (0-59)
 */
Value* zen_datetime_get_second(const Value* timestamp_value) {
    if (!timestamp_value || timestamp_value->type != VALUE_NUMBER) {
        return value_new_number(0);
    }
    
    time_t timestamp = (time_t)timestamp_value->as.number;
    struct tm* timeinfo = localtime(&timestamp);
    
    if (!timeinfo) {
        return value_new_number(0);
    }
    
    return value_new_number((double)timeinfo->tm_sec);
}

/**
 * @brief Get day of week from timestamp (0=Sunday, 6=Saturday)
 * @param timestamp_value Timestamp to extract day of week from
 * @return Day of week as number value (0-6)
 */
Value* zen_datetime_get_weekday(const Value* timestamp_value) {
    if (!timestamp_value || timestamp_value->type != VALUE_NUMBER) {
        return value_new_number(0);
    }
    
    time_t timestamp = (time_t)timestamp_value->as.number;
    struct tm* timeinfo = localtime(&timestamp);
    
    if (!timeinfo) {
        return value_new_number(0);
    }
    
    return value_new_number((double)timeinfo->tm_wday);
}

/**
 * @brief Check if year is a leap year
 * @param year_value Year to check
 * @return Boolean indicating if year is leap year
 */
Value* zen_datetime_is_leap_year(const Value* year_value) {
    if (!year_value || year_value->type != VALUE_NUMBER) {
        return value_new_boolean(false);
    }
    
    int year = (int)year_value->as.number;
    
    // Leap year calculation: divisible by 4, except century years unless divisible by 400
    bool is_leap = (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
    
    return value_new_boolean(is_leap);
}

/**
 * @brief Create duration from days
 * @param days_value Number of days
 * @return Duration in seconds as number value
 */
Value* zen_datetime_days(const Value* days_value) {
    if (!days_value || days_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    double days = days_value->as.number;
    return value_new_number(days * SECONDS_PER_DAY);
}

/**
 * @brief Create duration from hours
 * @param hours_value Number of hours
 * @return Duration in seconds as number value
 */
Value* zen_datetime_hours(const Value* hours_value) {
    if (!hours_value || hours_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    double hours = hours_value->as.number;
    return value_new_number(hours * SECONDS_PER_HOUR);
}

/**
 * @brief Create duration from minutes
 * @param minutes_value Number of minutes
 * @return Duration in seconds as number value
 */
Value* zen_datetime_minutes(const Value* minutes_value) {
    if (!minutes_value || minutes_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    double minutes = minutes_value->as.number;
    return value_new_number(minutes * SECONDS_PER_MINUTE);
}

/**
 * @brief Get start of day timestamp (midnight) for given timestamp
 * @param timestamp_value Timestamp to get start of day for
 * @return Start of day timestamp
 */
Value* zen_datetime_start_of_day(const Value* timestamp_value) {
    if (!timestamp_value || timestamp_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    time_t timestamp = (time_t)timestamp_value->as.number;
    struct tm* timeinfo = localtime(&timestamp);
    
    if (!timeinfo) {
        return value_new_number(0.0);
    }
    
    // Set to start of day (00:00:00)
    timeinfo->tm_hour = 0;
    timeinfo->tm_min = 0;
    timeinfo->tm_sec = 0;
    
    time_t start_of_day = mktime(timeinfo);
    return value_new_number((double)start_of_day);
}

/**
 * @brief Get end of day timestamp (23:59:59) for given timestamp
 * @param timestamp_value Timestamp to get end of day for
 * @return End of day timestamp
 */
Value* zen_datetime_end_of_day(const Value* timestamp_value) {
    if (!timestamp_value || timestamp_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    time_t timestamp = (time_t)timestamp_value->as.number;
    struct tm* timeinfo = localtime(&timestamp);
    
    if (!timeinfo) {
        return value_new_number(0.0);
    }
    
    // Set to end of day (23:59:59)
    timeinfo->tm_hour = 23;
    timeinfo->tm_min = 59;
    timeinfo->tm_sec = 59;
    
    time_t end_of_day = mktime(timeinfo);
    return value_new_number((double)end_of_day);
}

/**
 * @brief Convert timestamp to UTC
 * @param timestamp_value Local timestamp
 * @return UTC timestamp as number value
 */
Value* zen_datetime_to_utc(const Value* timestamp_value) {
    if (!timestamp_value || timestamp_value->type != VALUE_NUMBER) {
        return value_new_number(0.0);
    }
    
    time_t timestamp = (time_t)timestamp_value->as.number;
    struct tm* utc_time = gmtime(&timestamp);
    
    if (!utc_time) {
        return value_copy(timestamp_value);
    }
    
    time_t utc_timestamp = timegm(utc_time);
    return value_new_number((double)utc_timestamp);
}

/**
 * @brief Get timezone offset in seconds
 * @return Timezone offset from UTC in seconds
 */
Value* zen_datetime_timezone_offset(void) {
    time_t now = time(NULL);
    struct tm* local_time = localtime(&now);
    struct tm* utc_time = gmtime(&now);
    
    if (!local_time || !utc_time) {
        return value_new_number(0.0);
    }
    
    time_t local_timestamp = mktime(local_time);
    time_t utc_timestamp = timegm(utc_time);
    
    double offset = difftime(local_timestamp, utc_timestamp);
    return value_new_number(offset);
}