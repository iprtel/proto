#include <chrono>

#include <fmt/core.h>
#include "datetime.proto.h"
#include "datetime_fixed.proto.h"

datetime::DateTime now(){
    auto now = std::chrono::system_clock::now();
    auto today = std::chrono::floor<std::chrono::days>(now); // truncate to whole days
    std::chrono::year_month_day ymd{today};
    auto sec_from_noon = std::chrono::floor<std::chrono::seconds>(now - today) - std::chrono::seconds{86400/2};
    datetime::DateTime dt;
    dt.date.day = static_cast<unsigned>(ymd.day());
    dt.date.month = datetime::Month(static_cast<unsigned>(ymd.month()));
    dt.date.year = static_cast<int>(ymd.year());
    dt.seconds_from_noon = sec_from_noon.count();
    return dt;
}

int main(){
    datetime::DateTime dt = now();
    fmt::print("{}\n",dt);
    datetime::DateTime just_after_noon = {.date{.day=31, .month=datetime::Month::MONTH_DECEMBER, .year=2025}, .seconds_from_noon=1};
    datetime_fixed::DateTime just_after_noon_fixed = {.date{.day=31, .month=12, .year=2025}, .seconds_from_noon=1};
    fmt::print("{} == {}\n",just_after_noon, just_after_noon_fixed);
    auto encoded_after = encode_message(just_after_noon);
    auto encoded_after_fixed = encode_message(just_after_noon_fixed);
    fmt::print("{} == {}\n",encoded_after, encoded_after_fixed);

    datetime::DateTime just_before_noon = {.date{.day=31, .month=datetime::Month::MONTH_DECEMBER, .year=2025}, .seconds_from_noon=-1};
    datetime_fixed::DateTime just_before_noon_fixed = {.date{.day=31, .month=12, .year=2025}, .seconds_from_noon=-1};
    fmt::print("{} == {}\n",just_before_noon, just_before_noon_fixed);
    auto encoded_before = encode_message(just_before_noon);
    auto encoded_before_fixed = encode_message(just_before_noon_fixed);
    fmt::print("{} == {}\n",encoded_before, encoded_before_fixed);

    return 0;
}