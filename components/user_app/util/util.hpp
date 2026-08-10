
#pragma once

#include "util/pch.hpp"



// FORWARD DECLARATIONS =====================================================================================

namespace APP::util {

    // CONSTANTS ============================================================================================

    // MACROS ===============================================================================================

    // TYPES ================================================================================================

    // STATIC VARIABLES =====================================================================================

    // FUNCTION DECLARATION =================================================================================

    // TEMPLATE DECLARATION =================================================================================

    // @brief Converts a string to a numeric value of the specified type.
    //        Uses stringstream for the conversion, supporting all standard numeric types.
    // @tparam T The numeric type to convert to (e.g., int, float, double).
    // @param [str] The string representation of the number.
    // @return The numeric value parsed from the string.
    template <typename T>
    T str_to_num(const std::string& str);


    // @brief Given a string representing a variable access chain (e.g., "object1->object2.variable"),
    //          this function extracts and returns the name of the variable ("variable" in this example).
    //          The extraction process considers both "->" and "." as delimiters for nested access.
    // @param [input] The input string representing the variable access chain.
    // @return A string containing the name of the variable extracted from the input string.
    std::string extract_variable_name(const std::string& input);


    // @brief Creates a string consisting of multiple indentation levels of spaces.
    //        The total number of spaces is calculated as multiple_of_indenting_spaces * num_of_indenting_spaces.
    // @param [multiple_of_indenting_spaces] The number of indentation levels to generate.
    // @param [num_of_indenting_spaces] The number of spaces per indentation level (default is 2).
    // @return A string containing the calculated number of space characters.
    std::string string_add_spaces(const u32 multiple_of_indenting_spaces, u32 num_of_indenting_spaces = 2);


    // @brief Measures the indentation level of a string by counting leading spaces.
    //        The indentation level is calculated by dividing the number of leading spaces
    //        by the specified number of spaces per indentation level.
    // @param [str] The input string whose indentation is to be measured.
    // @param [num_of_indenting_spaces] The number of spaces per indentation level (default is 2).
    // @return The indentation level as an integer (number of indentation units).
    u32 string_measure_indentation(const std::string& str, u32 num_of_indenting_spaces = 2);


    // @brief Converts a string to a boolean value.
    //        Returns true if the string equals "true" (case-sensitive), false otherwise.
    // @param [string] The string to convert.
    // @return true if the string is "true", false otherwise.
    FORCE_INLINE constexpr bool str_to_bool(const std::string& string) { return (string == "true") ? true : false; }


    // @brief Converts a boolean value to a string.
    //        Returns "true" for true values and "false" for false values.
    // @param [boolean] The boolean value to convert.
    // @return [const char*] "true" if the boolean value is true, "false" otherwise.
    FORCE_INLINE constexpr const char *bool_to_str(bool boolean) { return boolean ? "true" : "false"; }


    // @brief Converts a value of type T to its string representation.
    //        Can handle conversion from various types such as: arithmetic types, boolean, glm::vec2,
    //        glm::vec3, glm::vec4, ImVec2, ImVec4, and glm::mat4, as well as custom types like version,
    //        system_time, filesystem::path, and UUID. If the input value type is not supported,
    // @param [src_value] The value to be converted to a string.
    // @param [dest_string] Reference to a string that will receive the conversion result.
    // @tparam T The type of the value to be converted.
    // @return None
    template <typename T>
    constexpr void to_string(const T& src_value, std::string& dest_string);


    // @brief Converts a value of type T to its string representation and returns it.
    //        This is a convenience wrapper around the void version of to_string.
    // @tparam T The type of the value to be converted.
    // @param [src_value] The value to be converted to a string.
    // @return A string representing the input value.
    template <typename T>
    constexpr std::string to_string(const T& src_value);


    // @brief Converts a string representation to a value of type T.
    //        Can handle conversion into various types such as: arithmetic types, boolean, glm::vec2,
    //        glm::vec3, glm::vec4, ImVec2, ImVec4, and glm::mat4, as well as custom types like version,
    //        system_time, filesystem::path, and UUID. If the input value type is not supported,
    //        a DEBUG_BREAK() is triggered.
    // @param [src_string] The string to be converted to a value.
    // @param [dest_value] Reference to the variable that will store the converted value.
    // @tparam T The type of the value the string should be converted to.
    // @return None
    template <typename T>
    constexpr void from_string(const std::string& src_string, T& dest_value);


    // @brief Converts a string representation to a value of type T and returns it.
    //        This is a convenience wrapper around the void version of from_string.
    // @tparam T The type of the value the string should be converted to.
    // @param [src_string] The string to be converted to a value.
    // @return The value of type T converted from the string.
    template <typename T>
    constexpr T from_string(const std::string& src_string);

    // CLASS DECLARATION ====================================================================================

}

#include "util/util.inl"
