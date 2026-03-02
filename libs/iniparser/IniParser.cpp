#include "IniParser.h"

IniParser::IniParser(std::string str) : file_name(str) {
    parseFile(this->file_name);  
}
IniParser::~IniParser() {
    if (this->file.is_open()) {
        this->file.close();
    }
}

void IniParser::parseFile(std::string &f_name) {
    this->file.open(f_name);
    if (!file) {throw std::invalid_argument(f_name + " could not be opened");}
    std::string string{};
    SectionData currentSection{};

    while (getline(this->file, string)) {
        ++line_count;
        if (string.empty()) { //пропускаем пустую строку сразу
            continue;
        }
        trimString(string);
        removeComment(string); 
            //удаляем пробелы, табуляции, пр.; удаляем комментарии            
        
        if (string.empty()) { //пустая строка или строка после удаления комментариев
            continue;
        } else if (string[0] == '[' && string[string.size() - 1] == ']' && !checkDelim(string, '=') && !checkDubles(string, '[',']')) { //проверяем если секция ограничена "[]", нет ли знака "=", есть ли дубли "[]"
            currentSection.name.clear();
            
            auto end = string.find_first_of(']');
            if (end != std::string::npos) {
                std::string read_name = string.substr(1, end - 1); //откидываем "[]" и записываем имя секции из текущей строки
                currentSection.name = read_name;
                auto it = std::find_if(SectionsData.begin(), SectionsData.end(), [&read_name](SectionData &a) {return a.name == read_name;}); //проверяем вектор структур на наличие имени секции, если не найдено указывает на .end()
                if (it == SectionsData.end()) { //не найдено - добавляем в вектор структур
                    SectionsData.push_back(currentSection);
                }//если нашли то ничего не пишем, так как уже есть такая секция в массиве - else {it->name = read_name;} 
            }
            else {
                throw std::invalid_argument("\tLine " + std::to_string(line_count) + ". Error in syntax. Check '[]'\n");
            }
        } else if (!string.empty() && !checkDelim(string, '[', ']') && !checkDubles(string, '=')) { //начинается строка с данными переменных, проверяем нет ли "[]", есть ли дубли "="
            currentSection.var_val.clear();
            const auto end = string.find_first_of("=");
            if (end != std::string::npos) { 
                std::string _var = string.substr(0, end);
                std::string _value = string.substr(end + 1);
                std::string read_name = currentSection.name;
                auto it = std::find_if(SectionsData.begin(), SectionsData.end(), [&read_name](SectionData &a) {return a.name == read_name;});
                if (it->var_val.find(_var) == it->var_val.end()){
                    it->var_val.insert({_var, _value});
                } else {
                    it->var_val[_var]=_value;
                }
            } else { //если не нашли "="
                throw std::invalid_argument("\tLine " + std::to_string(line_count) + ". Error in syntax.  Check '='\n");
            }
        } else {
            throw std::invalid_argument("\tLine " + std::to_string(line_count) + ". Error in syntax.\n");
        }
    }
}

SectionData* IniParser::getSecPtr(const std::string& section_name) { //функция возвращения указателя на позицию имени_секции в массиве структур
    std::vector<SectionData>::iterator it = std::find_if (SectionsData.begin(), SectionsData.end(), [section_name](const SectionData& sect) {return sect.name.compare(section_name) == 0;} );
    return it != SectionsData.end() ? &*it : nullptr;
}

void IniParser::trimString(std::string &str) {   
    std::string::size_type begin = str.find_first_not_of("\t\n\v\f\r");
    std::string::size_type end = str.find_last_not_of("\t\n\v\f\r");
    str = str.substr(begin, end-begin + 1);
    auto noSpaceEnd = std::remove(str.begin(), str.end(), ' ');
    str.erase(noSpaceEnd, str.end());
}

void IniParser::removeComment(std::string& str) {
    auto commentPosition = str.find(";");
    if (std::string::npos != commentPosition) {
        str = str.substr(0, commentPosition);
    }
}

bool IniParser::checkDelim (const std::string &_str, char _a) { //проверка разделителя в строке
    auto check = _str.find_first_of(_a);
    if (check != std::string::npos) {
        return true;
    }
    return false;
}
bool IniParser::checkDelim (const std::string &_str, char _a, char _b) { //проверка двух разделителей в строке
    auto check1 = _str.find_first_of(_a), check2 = _str.find_first_of(_b);
    if (check1 != std::string::npos || check2 != std::string::npos) {
        return true;
    }
    return false;
}

bool IniParser::checkDubles (const std::string &_str, char _a) { //проверка дублей разделителя в строке
    auto check_left = _str.find_first_of(_a), check_right = _str.find_last_of(_a);
    if (check_left != check_right) {
        return true;
    }
    return false;
}

bool IniParser::checkDubles (const std::string &_str, char _a, char _b) { //проверка дублей двух разделителей в строке
    auto check_left = _str.find_first_of(_a), check_right = _str.find_last_of(_a);
    if (check_left != check_right) {
        return true;
    }

    check_left = _str.find_first_of(_b), check_right = _str.find_last_of(_b);
    if (check_left != check_right) {
        return true;
    }

    return false;
}

const std::string IniParser::getKeyValue(const std::string& _sec_name, const std::string& _var_name) {
    SectionData* section = getSecPtr(_sec_name);
    if (section != nullptr) {
        const auto it = section->var_val.find(_var_name);
        if (it != section->var_val.end()) {
            if (it->second != "")
                return it->second;
            else
                throw std::invalid_argument("Variable " + _var_name + " in " + _sec_name + " has no value");
        }
        else {
            throw std::invalid_argument("Variable " + _var_name + " was not found in " + _sec_name);
        }
    }
    throw std::invalid_argument("Such section does not exist");
}

std::string IniParser::getKeyValue(const std::string &request) {
    if (!(checkDelim(request, '.') && !checkDubles(request, '.'))) {
        throw std::invalid_argument("Incorect request syntax"); 
    }

    std::stringstream ss(request);
    std::vector<std::string> out;
    std::string s;

    while (std::getline(ss, s, '.')) {
        out.push_back(s);
    }
    std::string value = getKeyValue(out[0],out[1]);
    return value;
}