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

void CSVtoXLTABularConverter::convert()
{
    std::setlocale(LC_ALL, "Russian"); // Set locale to the user's environment default
    auto table = csv_parser_->parse_all();

    auto header = table[1];
    int num_columns = static_cast<int>(header.size()); 
    std::cout << "num_columns: " << num_columns << "\n";
    
    int quantity, column_start, column_end;
    column_start =  1;
    if (num_columns > max_columns_) {
        column_end = max_columns_;
        quantity = (num_columns + max_columns_ - 2)/max_columns_;
        std::cout << "quantity: " << quantity << "\n\n";
    } else {
        column_end = num_columns;
        quantity = 1;
    }
    std::string header_line;

    for (int i = 1; i <= quantity; i++) {
        // Render header line
        std::cout << "column_start: " << column_start << " column_end: " << column_end << "\n";
        header_line = headerLineRender(column_start, column_end, header);
        std::cout << header_line << std::endl;
        // LaTeX tabular format
        tableRender(column_start, column_end, table, header_line);

        column_start += max_columns_;
        column_end += max_columns_;
        if (column_end > num_columns) {
            column_end = (num_columns - 1);
        }
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

void CSVtoXLTABularConverter::tableRender(int start_cell, int end_cell, const std::map<int, std::vector<std::string>>& table_, const std::string& header_line_) {
    auto table_width = end_cell - start_cell + 1;
    latex_string_ += "\\setlength\\LTleft{0cm} % Adjust the value as needed\n"
                    "\\begin{xltabular}{180mm}{|c|*{" + std::to_string(table_width) + "}{m{8.5mm}|}}\n"
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
