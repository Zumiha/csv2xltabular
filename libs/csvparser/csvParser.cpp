#include "csvParser.h"

CSVParser::CSVParser(const std::string& filename, char delimiter)
    : file_(filename), delimiter_(delimiter), line_num_(0) {
    if (!file_.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
}

// Parse entire CSV file into map structure
// map with row number and vector<string> of values
std::map<int, std::vector<std::string>> CSVParser::parse_all(int start_row, int start_col){
    std::map<int, std::vector<std::string>> table;
    reset_stream(); // Ensure starting from beginning

    // Read all lines into map
    while (auto row = next_row(start_row, start_col)) {
        table[static_cast<int>(row->line_number)] = std::move(row->fields);
    }

    if(table.empty()) {
        throw std::runtime_error("No data found from row " + std::to_string(start_row) + ", col " + std::to_string(start_col));
    }

    return table;
}

std::optional<dataformat::Row> CSVParser::next_row(int start_row, int start_col) {
    std::string line;
    while (std::getline(file_, line)) {
        ++line_num_;
        if (static_cast<int>(line_num_) < start_row) continue; // skip early rows cheaply

        dataformat::Row row;
        row.line_number = line_num_;
        row.fields = parse_line(line, start_col);  // parse only needed columns
        return row;
    }
    return std::nullopt;
}

// RFC 4180 state machine: handles quotes, embedded delimiters, escaped quotes
std::vector<std::string> CSVParser::parse_line(const std::string& line, int start_col) {
    std::vector<std::string> fields;
    std::string field;
    bool in_quotes = false;
    int col = 0; // Track column index for start_col
    
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        
        if (in_quotes) {
            if (c == '"') {
                // Check for escaped quote (double quote)
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    field += '"';
                    ++i; // Skip next quote
                } else {
                    in_quotes = false; // End of quoted field
                }
            } else {
                field += c;
            }
        } else {
            if (c == '"') {
                in_quotes = true;
            } else if (c == delimiter_) {
                if (col >= start_col) fields.push_back(field); // accumulate only needed cols
                field.clear();
                ++col;
            } else {
                field += c;
            }
        }
    }
    
    if (col >= start_col) fields.push_back(field); // Add last field
    
    if (in_quotes) {
        throw std::runtime_error("Unterminated quoted field at line " + std::to_string(line_num_));
    }
    
    return fields;
}

void CSVParser::reset_stream() {
    file_.clear(); // Clear EOF flag
    file_.seekg(0); // Rewind to beginning
    line_num_ = 0;
}
