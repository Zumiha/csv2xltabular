#ifndef CSV2XTABULAR_H
#define CSV2XTABULAR_H

#include "csvParser.h"
#include "IniParser.h"

#include <iostream>
#include <fstream>
#include <string>

struct TablesRows {
    bool remain_table = false;
    int col_start;
    int col_end;
};

class TableConfig {
public:
    std::vector<TablesRows> tables_rows_config;
    
    int max_columns = 12; // Render default max columns per page
    int remdnr_min = 8; // Minimum remainder

    int table_width = 180; // mm, total width of the table in LaTeX
    int columns_sum_width = 102; // mm, sum of column widths (excluding the first column)
    float table_column_width; // mm, width of each column in the main tables

};

class CSVtoXLTABularConverter {
public:
    CSVtoXLTABularConverter() = delete;
    CSVtoXLTABularConverter(const CSVtoXLTABularConverter&) = delete;

    CSVtoXLTABularConverter(const std::string& csv_filename, const std::string& ini_filename); 
    ~CSVtoXLTABularConverter();      

    // Convert CSV to LaTeX tabular format based on INI configuration
    void convert();  
    void exportToFile(const std::string& output_filename = "wt_table.tex");
    void exportToCSV(const std::string& output_filename = "debug.csv") const {
        // for (const auto& [k, v] : parsed_table_)
        // {
        //     std::cerr << "row " << k << ": " << v.size() << " fields | first='" << (!v.empty() ? v[0] : "<empty>") << "'\n";
        // }
        csv_parser_->export_csv(parsed_table_, output_filename);
    }    
    
    private:

    CSVParser* csv_parser_;
    IniParser* ini_parser_;
    
    int start_colum_ = 0;
    int start_row_ = 0;
    
    TableConfig table_config_;

    // Parsed table as map
    std::map<int, std::vector<std::string>> parsed_table_;
    
    std::string latex_string_ = ""; // LaTeX tabular format string
    
    std::string headerLineRender(
        int start_cell, 
        int end_cell, 
        const std::vector<std::string>& header_
    );
    void tableRender(
        int _table_width, 
        float _column_width, 
        int table_size, 
        int start_cell, 
        int end_cell, 
        const std::map<int, std::vector<std::string>>& table_, 
        const std::string& header_line_
    );
    TableConfig calculateTableConfig(int _header_size);
};

#endif // CSV2XTABULAR_H