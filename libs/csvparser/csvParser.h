#ifndef CSV_PARSER_H
#define CSV_PARSER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <optional>

// CSVParser: Streaming parser for RFC 4180 compliant CSV files
// Uses state machine for quoted field handling
namespace dataformat {
    struct Row {
        std::vector<std::string> fields;
        size_t line_number;
    };
}

class CSVParser {
public:
    explicit CSVParser(const std::string& filename, char delimiter = ',');

    // Parse entire CSV file and return indexed table
    // Key: row number (1-based), Value: vector of field strings
    // Throws runtime_error on parse failures
    std::map<int, std::vector<std::string>> parse_all();


    // Parse next row from CSV file
    // Returns nullopt when EOF reached
    std::optional<dataformat::Row> next_row();
    size_t current_line() const { return line_num_; }

private:
    std::ifstream file_;
    char delimiter_;
    size_t line_num_;

    // State machine parser for handling quoted fields with embedded delimiters/newlines
    std::vector<std::string> parse_line(const std::string& line);

    // Reset file stream to beginning
    void reset_stream();
};

#endif // CSV_PARSER_H