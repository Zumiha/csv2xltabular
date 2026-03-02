#ifndef CSV2XTABULAR_H
#define CSV2XTABULAR_H

#include "csvParser.h"
#include "IniParser.h"

#include <iostream>
#include <fstream>
#include <string>

class CSVtoXLTABularConverter {
public:
    CSVtoXLTABularConverter() = delete;
    CSVtoXLTABularConverter(const CSVtoXLTABularConverter&) = delete;

    CSVtoXLTABularConverter(const std::string& csv_filename, const std::string& ini_filename); 
    ~CSVtoXLTABularConverter();      

    // Convert CSV to LaTeX tabular format based on INI configuration
    void convert();  
    void exportToFile(const std::string& output_filename = "csv_table.tex");

    
    private:
    CSVParser* csv_parser_;
    IniParser* ini_parser_;
    
    std::string lang_ = "rus"; // Default language
    int max_columns_ = 12; // Render default max columns per page
    std::string latex_string_ = ""; // LaTeX tabular format string
    
    std::string headerLineRender(int start_cell, int end_cell, const std::vector<std::string>& header_);
    void tableRender(int start_cell, int end_cell, const std::map<int, std::vector<std::string>>& table_, const std::string& header_line_);
};

#endif // CSV2XTABULAR_H