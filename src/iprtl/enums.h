#pragma once

#include <array>
#include <string_view>
#include <optional>
#include <algorithm>
#include <span>
#include <fmt/core.h>

namespace iprtl {
namespace enums_detail{
    constexpr std::optional<std::string_view> find_name(auto value, auto names){
        auto entry = std::ranges::find_if(names,[value](auto entry){return entry.value == value;});
        if (entry != names.end()){
            return entry->name;
        }
        return {};
    }


    template<typename EnumType>
    struct EnumEntry {
        EnumType value{};
        std::string_view name{};
    };

    using namespace std::literals::string_view_literals;

    constexpr bool is_whitechar(char c){
        constexpr auto chars = " \t\r\n"sv;
        auto found = chars.find_first_of(c);
        return found != std::string_view::npos;
    }

    constexpr std::size_t skip_whitespace(std::string_view const args, std::size_t index) {
        while(index < args.size() && is_whitechar(args[index])){index++;}
        return index;
    }

    template<typename Function>
    constexpr void for_each(std::string_view args, Function func) {
        for(auto index=0; index < args.size(); index++) {
            index = skip_whitespace(args, index);
            auto start = index;
            while(index < args.size() && args[index] != ','){index++;}
            func(args.substr(start, index-start));
        }
    }

    constexpr std::size_t count_entries(std::string_view args) {
        std::size_t result{0};
        auto counter = [&](std::string_view){
            result++;
        };
        enums_detail::for_each(args, counter);
        return result;
    }

    constexpr bool is_idchar(char c){
        constexpr auto chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ_"
                               "abcdefghijklmnopqrstuvwxyz0123456789"sv;
        auto found = chars.find_first_of(c);
        return found != std::string_view::npos;
    }
    constexpr bool is_digit(char c){
        constexpr auto chars = "0123456789"sv;
        auto found = chars.find_first_of(c);
        return found != std::string_view::npos;
    }

    template<typename T>
    constexpr std::tuple<bool,T> has_value(std::string_view args, T index){
        index = skip_whitespace(args,index);
        bool found_value = index < args.size() && args[index++] == '=';
        return {found_value, index};
    } 

    constexpr auto read_value(std::string_view args, auto index){
        index = skip_whitespace(args,index);
        auto numeric_value{0};
        auto negative = args[index] == '-';
        if(negative){ index++; }

        while(index < args.size() && is_digit(args[index])) {
            numeric_value *= 10;
            numeric_value += (args[index++] - '0');
        }
        if (negative) return -numeric_value;
        else          return  numeric_value;
    }

    template<typename EnumEntry>
    constexpr EnumEntry read_entry(std::string_view args, auto numeric_value){
        using EnumType = decltype(EnumEntry::value);
        auto stop = std::distance(args.begin(),
                        std::ranges::find_if_not(args,is_idchar));
        auto [found_value, index] = has_value(args, stop);
        if (found_value){
            numeric_value = read_value(args,index);
        } 
        return {EnumType{numeric_value}, args.substr(0,stop)};
    }

    template<typename EntriesArray>
    constexpr EntriesArray create_entries_array(std::string_view args){
        using EnumEntry = EntriesArray::value_type;
        using EnumType = decltype(EnumEntry::value);
        using EnumUnderlying = std::underlying_type<EnumType>::type;
        EntriesArray result{};
        std::span<EnumEntry> result_span{result};
        EnumUnderlying numeric_value{0};
        auto read_name = [&](std::string_view arg){
            result_span[0] = read_entry<EnumEntry>(arg, numeric_value);
            numeric_value = static_cast<EnumUnderlying>(result_span[0].value) + 1;
            result_span = result_span.subspan(1);
        };
        for_each(args, read_name);
        return result;
    }

    constexpr std::size_t get_names_length(std::string_view args) {
        std::size_t total_length{0};
        auto counter = [&](std::string_view arg){
            total_length += std::distance(arg.begin(),
                            std::ranges::find_if_not(args,is_idchar));
            total_length++; // For the terminator
        };
        enums_detail::for_each(args, counter);
        return total_length;
    }


}
}

#define ENUMS(ename, etype,...)                                                \
    enum class ename : etype {__VA_ARGS__};                                    \
    constexpr auto ename##_Names{#__VA_ARGS__};                                \
    using ename##_EntryType = iprtl::enums_detail::EnumEntry<ename>;                   \
    using ename##_EntriesArray =                                                       \
      std::array<ename##_EntryType,iprtl::enums_detail::count_entries(ename##_Names)>; \
    constexpr ename##_EntriesArray ename##_Entries =                                   \
      iprtl::enums_detail::create_entries_array<ename##_EntriesArray>(ename##_Names);  \
      inline std::string format_as(ename value){                             \
        auto value_string = iprtl::enums_detail::find_name(value, ename##_Entries);   \
        if (value_string) { return std::string(*value_string); }                  \
        return std::string("unknown_") + std::to_string(static_cast<int>(value));  \
    }                                                                          \
    auto& operator<<(auto& stream, ename value){                               \
        auto value_string = iprtl::enums_detail::find_name(value, ename##_Entries);   \
        if (value_string) { stream << *value_string; }                         \
        return stream;                                                         \
    }                                                                          \

