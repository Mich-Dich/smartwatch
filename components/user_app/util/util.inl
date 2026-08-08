#pragma once

#include <string>



// FORWARD DECLARATIONS ================================================================================================

namespace APP::util {

    // CONSTANTS =======================================================================================================

    // MACROS ==========================================================================================================

    // TYPES ===========================================================================================================

    // STATIC VARIABLES ================================================================================================

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================

    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================

    // TEMPLATE IMPLEMENTATION =========================================================================================

    template <typename T>
    T str_to_num(const std::string& str) {

        std::istringstream ss(str);
        T num{};
        ss >> num;
        return num;
    }


    template <typename... Args>
    std::string to_string(Args&& ...args) {
        std::ostringstream oss;
        (oss << ... << std::forward<Args>(args));
        return oss.str();
    }


    template <typename T>
    constexpr void to_string(const T& src_value, std::string& dest_string) {

        if constexpr (std::is_same_v<T, bool>) {

            dest_string = bool_to_str(src_value);
            return;
        }

        // else if constexpr (std::is_same_v<T, version>) {

        //     std::ostringstream oss;
        //     oss << src_value.major << ' ' << src_value.minor << ' ' << src_value.patch;
        //     dest_string = oss.str();
        //     return;
        // }

        // else if constexpr (std::is_same_v<T, system_time>) {

        //     std::ostringstream oss;
        //     oss << (u16)src_value.year
        //         << ' ' << (u16)src_value.month
        //         << ' ' << (u16)src_value.day
        //         << ' ' << (u16)src_value.day_of_week
        //         << ' ' << (u16)src_value.hour
        //         << ' ' << (u16)src_value.minute
        //         << ' ' << (u16)src_value.secund
        //         << ' ' << (u16)src_value.millisecond;
        //     dest_string = oss.str();
        //     return;
        // }

        else if constexpr (std::is_same_v<T, std::filesystem::path>) {

            dest_string = src_value.string();
            return;
        }

        // else if constexpr (std::is_same_v<T, UUID>) {

        //     dest_string = std::to_string((u64)src_value);
        //     return;
        // }

        else if constexpr (std::is_arithmetic_v<T>) {

            dest_string = std::to_string(src_value);
            return;
        }

        else if constexpr (std::is_convertible_v<T, std::string>) {

            // LOG(Fatal, "called: to_string() with string");
            dest_string = src_value;
            // std::replace(dest_string.begin(), dest_string.end(), ' ', '%');
            std::replace(dest_string.begin(), dest_string.end(), '\n', '$');
            return;
        }

        else if constexpr (std::is_enum_v<T>) {

            dest_string = std::to_string(static_cast<std::underlying_type_t<T>>(src_value));
            return;
        }

        // else
        //     DEBUG_BREAK(); // Input value is not supported
    }


    template <typename T>
    constexpr std::string to_string(const T& src_value) {

        std::string dest_string;

        to_string<T>(src_value, dest_string);
        return dest_string;
    }


    template <typename T>
    constexpr void from_string(const std::string& src_string, T& dest_value) {

        if constexpr (std::is_same_v<T, bool>) {

            dest_value = util::str_to_bool(src_string);
            return;
        }

        // else if constexpr (std::is_same_v<T, version>) {

        //     std::istringstream iss(src_string);
        //     iss >> dest_value.major >> dest_value.minor >> dest_value.patch;
        //     return;
        // }

        // else if constexpr (std::is_same_v<T, system_time>) {

        //     std::istringstream iss(src_string);
        //     iss >> dest_value.year >> dest_value.month >> dest_value.day >> dest_value.day_of_week >> dest_value.hour >> dest_value.minute >> dest_value.secund >> dest_value.millisecond;
        //     return;
        // }

        else if constexpr (std::is_same_v<T, std::filesystem::path>) {

            dest_value = src_string;
            return;
        }

        // else if constexpr (std::is_same_v<T, UUID>) {

        //     dest_value = util::str_to_num<u64>(src_string);
        //     return;
        // }

        else if constexpr (std::is_same_v<T, const char *>) {

            std::string temp_str = src_string;
            std::replace(temp_str.begin(), temp_str.end(), '$', '\n');
            dest_value = temp_str.c_str();
            return;
        }

        else if constexpr (std::is_arithmetic_v<T>) {

            dest_value = util::str_to_num<T>(src_string);
            return;
        }

        else if constexpr (std::is_convertible_v<T, std::string>) {

            dest_value = src_string; // <= HERE
            std::replace(dest_value.begin(), dest_value.end(), '$', '\n');
            return;
        }

        else if constexpr (std::is_enum_v<T>) {

            dest_value = static_cast<T>(std::stoi(src_string));
            return;
        }

        // else
        //     DEBUG_BREAK(); // Input value is not supported
    }


    template <typename T>
    constexpr T from_string(const std::string& src_string) {

        T dest_value;
        from_string<T>(src_string, dest_value);
        return dest_value;
    }

    // TEMPLATE CLASS IMPLEMENTATION ===================================================================================

    // TEMPLATE CLASS PUBLIC ===========================================================================================

    // TEMPLATE CLASS PROTECTED ========================================================================================

    // TEMPLATE CLASS PRIVATE ==========================================================================================

}
