
#include "util/pch.hpp"
#include "util.hpp"


// FORWARD DECLARATIONS ================================================================================================

namespace APP::util {

    // TYPES ===========================================================================================================

    // CONSTANTS =======================================================================================================

    // MACROS ==========================================================================================================

    // STATIC VARIABLES ================================================================================================

    // INTERNAL TEMPLATE DECLARATION ===================================================================================

    // INTERNAL FUNCTION DECLARATION ===================================================================================

    void extract_part_after_delimiter(std::string& dest, const std::string& input, const char* delimiter);

    void extract_part_befor_delimiter(std::string& dest, const std::string& input, const char* delimiter);

    // INTERNAL TEMPLATE IMPLEMENTATION ================================================================================

    // INTERNAL FUNCTION IMPLEMENTATION ================================================================================

    void extract_part_after_delimiter(std::string& dest, const std::string& input, const char* delimiter) {

        size_t found = input.find_last_of(delimiter);
        if (found != std::string::npos) {

            dest = input.substr(found + 1);
            return;
        }
        // delimiter is not found
    }


    void extract_part_befor_delimiter(std::string& dest, const std::string& input, const char* delimiter) {

        size_t found = input.find_last_of(delimiter);
        if (found != std::string::npos) {

            dest = input.substr(0, found);
            return;
        }
        // delimiter is not found
    }

    // TEMPLATE IMPLEMENTATION =========================================================================================

    // FUNCTION IMPLEMENTATION =========================================================================================

    std::string extract_variable_name(const std::string& input) {

        std::string result = input;
        extract_part_after_delimiter(result, input, "->");
        extract_part_after_delimiter(result, result, ".");
        return result;
    }


    std::string string_add_spaces(const u32 multiple_of_indenting_spaces, u32 num_of_indenting_spaces) {

        if (multiple_of_indenting_spaces == 0)
            return "";

        return std::string(multiple_of_indenting_spaces * num_of_indenting_spaces, ' ');
    }


    u32 string_measure_indentation(const std::string& str, u32 num_of_indenting_spaces) {

        u32 count = 0;
        for (char ch : str) {
            if (ch == ' ')
                count++;
            else
                break; // Stop counting on non-space characters
        }

        return count / num_of_indenting_spaces;
    }

    // CLASS IMPLEMENTATION ============================================================================================

    // CLASS PUBLIC ====================================================================================================

    // CLASS PROTECTED =================================================================================================

    // CLASS PRIVATE ===================================================================================================

}
