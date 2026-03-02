#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <map>
#include <exception>

struct SectionData {
    std::string name;
    std::map<std::string, std::string> var_val;
};

class IniParser {
public:
    IniParser(std::string str);
    ~IniParser();

    template <typename T> T getValue (const std::string &request_string) {
        T result{};
        std::string stringvalue = getKeyValue(request_string); //строку из имен секции и переменной в stringvalue

        if constexpr (std::is_same<int, T>::value) {
            result = std::stoi(stringvalue);
        } else if constexpr (std::is_same<double, T>::value) {
            result = std::stod(stringvalue);
        } else if constexpr (std::is_same<std::string, T>::value) {
            result = stringvalue;
        } else {
            static_assert(sizeof(T) == -1, "no implementation for this type!");
        }
        return result;        
    }

    std::string getValue (const std::string &_section, const std::string &_var) {
        const std::string var_value = getKeyValue(_section, _var); //строку из имен секции и переменной в stringvalue
        return var_value;        
    }

private:
    std::string request_line;
    std::string file_name;
    std::ifstream file;
    std::vector<SectionData> SectionsData{};
    int line_count = 0;


    void parseFile(std::string &f_name);

    SectionData* getSecPtr(const std::string& section_name);

    void trimString(std::string &str);
    void removeComment(std::string& str);

    bool checkDelim (const std::string &_str, char _a);
    bool checkDelim (const std::string &_str, char _a, char _b);

    bool checkDubles (const std::string &_str, char _a);
    bool checkDubles (const std::string &_str, char _a, char _b);

    const std::string getKeyValue(const std::string& _sec_name, const std::string& _var_name);
    std::string getKeyValue(const std::string &request);
};