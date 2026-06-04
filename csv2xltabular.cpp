#include "csv2xltabular.h"

CSVtoXLTABularConverter::CSVtoXLTABularConverter(const std::string &csv_filename, const std::string &ini_filename) {
    ini_parser_ = new IniParser(ini_filename);
    
    start_row_ = ini_parser_->getValue<int>("source_csv.start_row");
    start_colum_ = ini_parser_->getValue<int>("source_csv.start_column");

    table_config_.max_columns = ini_parser_->getValue<int>("table_settings.max_columns");
    table_config_.remdnr_min = ini_parser_->getValue<int>("table_settings.min_columns");
     
    csv_parser_ = new CSVParser(csv_filename);
    std::setlocale(LC_ALL, "Russian"); // Set locale to the user's environment default
    parsed_table_ = csv_parser_->parse_all(start_row_, start_colum_);

    switch (ini_parser_->getValue<int>("source_csv.type"))
    {
    case 1:
        convert_type_ = DataType::WtTable;
        break;
    case 2:
        convert_type_ = DataType::MtmSpreadSheet;
        break;    
    default:
        convert_type_ = DataType::Default;
        break;
    }
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
    for (int i = this->table_config_.max_columns; i >= 1; --i) {
        int k = effective_columns / i; 
        int r = effective_columns % i; 
        
        if ((effective_columns / i == 1 && r == 0) || effective_columns / i == 0) {
            std::cout << "CSV columns fit in one table: " << effective_columns;
            table_settings.tables_rows_config.push_back({false, 1, effective_columns});
            table_settings.table_column_width = static_cast<float>(table_settings.columns_sum_width) / table_settings.tables_rows_config[0].col_end;
            std::cout << " with column width: " << table_settings.table_column_width << " mm\n";
            return table_settings;    
        }

        if (r == 0 || r >= this->table_config_.remdnr_min) {
            std::cout << "\nNumber of tables with " << i << " columns: " << k << "\n";

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
    throw -1;
}

void CSVtoXLTABularConverter::convert()
{
    switch (convert_type_)
    {
    case DataType::WtTable:        
        std::cout << "Modding to WT table format\n";
        modWtTable();
        break;
    case DataType::MtmSpreadSheet:
        std::cout << "Modding to SpreadSheet format\n";
        modMtmSpSh();
        break;
    default:
        std::cout << "Chosen convert type \"" << to_string(convert_type_) << "\" not implemented.";
        break;
    }    
    table_converted_ = true;
}

void CSVtoXLTABularConverter::exportToFile(const std::string &output_filename) {    
    std::ofstream outfile(output_filename);
    if (!outfile.is_open()) {
        throw std::runtime_error("Failed to open output file: " + output_filename);
    }
    outfile << this->latex_string_;
    outfile.close();
}

void CSVtoXLTABularConverter::modDefault()
{
    // Default table conversion block
}

void CSVtoXLTABularConverter::modWtTable()
{
    // WT measurement table conversion block
    auto header = parsed_table_[1];
    int header_size = static_cast<int>(header.size()); 
    std::cout << "\nheader_size: " << header_size << "\n";

    int table_count, table_rows, column_start, column_end;
    column_start =  1;

    auto table_config_ = calculateTableConfig(header_size);

    std::string header_line;
    latex_string_ +="\\newcounter{tablefigure}[section]\n"
                    "\\renewcommand{\\thetablefigure}{\\thesection.\\arabic{tablefigure}}\n\n";  
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
            parsed_table_, 
            header_line
        );
    }
}

void CSVtoXLTABularConverter::modMtmSpSh()
{
    // Extract project data from csv
    std::vector<std::string> prj_field_header = {
        "Pprj", "Pwork", "Pallow", "Year", "WT", "D", "SMYS"};
    std::vector<size_t> prj_fields = {
        47, 48, 49, 50, 52, 53, 54};
    auto data_from_table = csv_parser_->extractTable(parsed_table_, prj_fields);
    
    auto prj_info = extractAndValidate(data_from_table);
    std::map<int, std::vector<std::string>> prj_info_table;

    prj_info_table[1] = prj_field_header;
    prj_info_table[2] = prj_info;
    csv_parser_->export_csv(prj_info_table, "prj_info.csv");
    
    // MTM SpreadSheet table conversion block
    // Remove columns not used in spreadsheet
    std::vector<size_t> columns_list = {62, 61, 60, 59, 58, 57, 56, 55, 54, 53,
                                        52, 51, 50, 49, 48, 47, 46, 45, 44, 40, 
                                        39, 38, 36, 35, 34, 33, 32, 31, 29, 28, 
                                        27, 26, 25, 24, 23, 21, 19, 18, 16, 15, 
                                        14, 13, 12, 11, 10, 9, 8, 6
                                        };
    
    csv_parser_->formatTable(parsed_table_, columns_list);
    // Merge columns
    mergeColumns(parsed_table_, 0, 8); // Merge lenght columns
}

bool CSVtoXLTABularConverter::isEmptyRow(const std::vector<std::string> &vec) {
    bool all_empty = true;
    for (const auto& s : vec)
        if (!s.empty() && s != "\"\"") all_empty = false;
    return all_empty;
}

std::vector<std::string> CSVtoXLTABularConverter::extractAndValidate(const std::map<int, std::vector<std::string>> &table)
{
    std::cout << "\nExtracting and validating project data.\n";

    std::vector<std::string> values; // first non-empty row captured here
    for (const auto& [key, row] : table) {
        if (isEmptyRow(row)) continue; // skip empty rows and \"\" rows
        if (values.empty()) {            
            values = row; // capture first non-empty row as reference
            continue;
        }

        // Subsequent non-empty rows: must match values exactly
        if (row.size() != values.size()) {
            std::ostringstream oss;
            oss << "Row " << key << ": column count mismatch ("
                << row.size() << " vs expected " << values.size() << ")";
            throw std::runtime_error(oss.str());
        }

        for (std::size_t i = 0; i < values.size(); ++i) {
            if (row[i] != values[i] ) {
                std::ostringstream oss;
                oss << "Row " << key << ", col " << i
                    << ": value mismatch (\"" << row[i]
                    << "\" vs expected \"" << values[i] << "\")";
                throw std::runtime_error(oss.str());
            }
        }        
    }
    return values;
}

void CSVtoXLTABularConverter::mergeColumns(std::map<int, std::vector<std::string>> &table, size_t primary_col, size_t secondary_col)
{   
    std::cout << "\nMerging columns " << primary_col << " and " << secondary_col << "\n";

    if (table[1].size() < std::max(primary_col, secondary_col) + 1) {
        std::ostringstream oss;
        oss << "Rows out of range for merge: row has only " << table[1].size() << " columns, but need at least " << std::max(primary_col, secondary_col) + 1;
        throw std::runtime_error(oss.str());
    }

    for (auto& [row, fields] : table) {        
        if (fields[primary_col].empty() || fields[primary_col] == "\"\"") {
            std::cout << "Processing row " << row << "\n";
            fields[primary_col] = fields[secondary_col]; // merge into empty primary
            continue;
        } 
        if (!fields[secondary_col].empty() && fields[secondary_col] != "\"\"") {
            std::ostringstream oss;
            oss << "Row " << row << ": cannot merge non-empty columns " << primary_col << " and " << secondary_col;
            throw std::runtime_error(oss.str());
        }
    }

    // Remove secondary column after merge
    for (auto& [row, fields] : table) {
        fields.erase(fields.begin() + secondary_col);
    }
}

std::string CSVtoXLTABularConverter::headerLineRender(int start_cell, int end_cell, const std::vector<std::string> &header_)
{
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
    latex_string_ +="\\setlength\\LTleft{0cm}\n"
                    "\\stepcounter{tablefigure}\n"
                    "\\noindent Таблица~\\thetablefigure: Измерения толщины стенки~\\vspace{-0.75em}\n"
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
    latex_string_ += "\\end{xltabular}%\n\\vspace{0em}\n\n";
}
