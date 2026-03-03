#include "csv2xltabular.h"

CSVtoXLTABularConverter::CSVtoXLTABularConverter(const std::string &csv_filename, const std::string &ini_filename) {
    ini_parser_ = new IniParser(ini_filename);
    lang_ = ini_parser_->getValue<std::string>("table_settings.lang");
    max_columns_ = ini_parser_->getValue<int>("table_settings.max_columns");
    csv_parser_ = new CSVParser(csv_filename);
}

CSVtoXLTABularConverter::~CSVtoXLTABularConverter()
{
    csv_parser_ = nullptr;
    delete csv_parser_;
    ini_parser_ = nullptr;
    delete ini_parser_;
}

TableConfig CSVtoXLTABularConverter::calculateTableConfig(int _header_size) {
    TableConfig table_settings{};
    auto effective_columns = _header_size - 1; std::cout << "\neffective_columns: " << effective_columns << "\n";
    for (int i = max_columns_; i >= 1; --i) {
        std::cout << "Counting loop: " << i << "\t";
        int k = effective_columns / i; std::cout << "k: " << k << "\t";
        int r = effective_columns % i; std::cout << "r: " << r << "\n";
        
        if ((effective_columns / i == 1 && r == 0) || effective_columns / i == 0) {
            std::cout << "CSV columns fit in one table: " << effective_columns;
            table_settings.tables_rows_config.push_back({false, 1, effective_columns});
            table_settings.table_column_width = static_cast<float>(table_settings.columns_sum_width) / table_settings.tables_rows_config[0].col_end;
            std::cout << " with column width: " << table_settings.table_column_width << " mm\n";
            return table_settings;    
        }

        if (r == 0 || r >= this->remdnr_min_) {
            std::cout << "\nNumber of tables with " << i << " columns: " << k << "\n";
            // table_settings.table_count = i; table_settings.max_columns = k;
            int col_start = 1, col_end = i;
            for (int j = 0; j < k; ++j) {
                table_settings.tables_rows_config.push_back({false, col_start, col_end});
                std::cout << "Table " << j + 1 << ": col_start = " << col_start << ", col_end = " << col_end << "\n";
                col_start += i;
                col_end += i;
            }


            if (r > 0) {
                table_settings.tables_rows_config.push_back({true, col_start, col_start + r - 1});
                std::cout << "Plus table with " << r << " columns: ";
                // table_settings.remain_columns = r;
                std::cout << "col_start = " << col_start << ", col_end = " << (col_start + r - 1) << "\n";
            }

            table_settings.table_column_width = static_cast<float>(table_settings.columns_sum_width) / table_settings.tables_rows_config[0].col_end;
            std::cout << " with column width: " << table_settings.table_column_width << " mm\n";

            std::cout << std::endl;
            return table_settings;  
        }
    }
    return table_settings; // Default return, should not reach here
}

void CSVtoXLTABularConverter::convert()
{
    std::setlocale(LC_ALL, "Russian"); // Set locale to the user's environment default
    auto table = csv_parser_->parse_all();

    auto header = table[1];
    int header_size = static_cast<int>(header.size()); 
    std::cout << "\nheader_size: " << header_size << "\n";

    int table_count, table_rows, column_start, column_end;
    column_start =  1;

    auto table_config_ = calculateTableConfig(header_size);

    std::string header_line;

    for (int i = 0; i < table_config_.tables_rows_config.size(); i++) {
        // Render header line
        int cell_start = table_config_.tables_rows_config[i].col_start;
        int cell_end = table_config_.tables_rows_config[i].col_end;
        int table_size = cell_end - cell_start + 1;

        std::cout << "\ncolumn_start: " << cell_start << " column_end: " << cell_end << "\n";
        header_line = headerLineRender(
            cell_start, 
            cell_end, 
            header
        );
        std::cout << header_line << std::endl;
        // LaTeX tabular format

        tableRender(
            table_config_.table_width, 
            table_config_.table_column_width,  
            table_size, 
            cell_start, 
            cell_end, 
            table, 
            header_line
        );
    } 
}

void CSVtoXLTABularConverter::exportToFile(const std::string &output_filename) {
    std::ofstream outfile(output_filename);
    if (!outfile.is_open()) {
        throw std::runtime_error("Failed to open output file: " + output_filename);
    }
    outfile << this->latex_string_;
    outfile.close();
}

std::string CSVtoXLTABularConverter::headerLineRender(int start_cell, int end_cell, const std::vector<std::string>& header_) {
    std::string line;
    for (auto i = start_cell; i <= end_cell; ++i) {
        line += header_[i];
        if (i < end_cell) {
            line += " & ";
        } else {
            line += "\\\\ \\hline\n";
        }
    }
    return line;
}

void CSVtoXLTABularConverter::tableRender(
    int _table_width, 
    float _column_width,  
    int table_size, 
    int start_cell, 
    int end_cell, 
    const std::map<int, std::vector<std::string>>& table_, 
    const std::string& header_line_
) {
    auto table_width = end_cell - start_cell + 1;
    latex_string_ += "\\setlength\\LTleft{0cm} % Adjust the value as needed\n"
                    "\\begin{xltabular}{" + std::to_string(_table_width) + "mm}{|c|*{" + std::to_string(table_width) + "}{m{" + std::to_string(_column_width) + "mm}|}}\n"
                    "\\hline\n"
                    "\\diagbox{час}{L,мм} & ";
    latex_string_ += header_line_;
    latex_string_ +="\\endfirsthead\n"
                    "\\multicolumn{" + std::to_string(table_width + 1) + "}{@{}l}{\\small\\sl продолжение на предыдущей странице}\\\\ \\hline\n"
                    "\\diagbox{час}{L,мм} & ";
    latex_string_ += header_line_;
    latex_string_ +="\\endhead\n"
                    "\\multicolumn{" + std::to_string(table_width + 1) + "}{r}{\\small\\sl продолжение на следующей странице}\\\\ \n"
                    "\\endfoot\n"
                    "\\endlastfoot\n";
    // Fill table rows
    for (const auto& [row_num, fields] : table_) {
        if (row_num == 1) {
            continue; // Skip header row
        }
        // Header Row input before each line
        latex_string_ += (fields[0] + " & ");

        for (size_t i = start_cell; i < end_cell + 1; ++i) {
            latex_string_ += fields[i];
            if (i < end_cell) {
                latex_string_ += " & ";
            } else {
                latex_string_ += "\\\\ \\hline\n";
            }

        }
    }
    latex_string_ += "\\end{xltabular}%\n\\vspace{-1em}\n";
}
