#include "csvParser.h"

CSVParser::CSVParser(const std::string& filename, char delimiter)
    : file_(filename), delimiter_(delimiter), line_num_(0) 
{
    if (!file_.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
}

// Parse entire CSV file into map structure
// map with row number and vector<string> of values
std::map<int, std::vector<std::string>> CSVParser::parse_all(int start_row, int start_col)
{
    // 1-based interface — reject zero or negative values
    if (start_row < 1 || start_col < 1) throw std::invalid_argument("start_row and start_col must be >= 1");

    std::map<int, std::vector<std::string>> table;
    reset_stream(); // Ensure starting from beginning

    // ── Phase 1: confirm parameters and reset state ──────────────────────────
    std::cerr << "[DBG] parse_all(start_row=" << start_row << ", start_col=" << start_col << ")\n";
    std::cerr << "[DBG] line_num_ after reset = " << line_num_ << "\n";
    std::cerr << "[DBG] expected to skip " << (start_row - 1) << " row(s)\n-----------------------------\n";

    // ── Phase 2: dummy skip loop ─────────────────────────────────────────────
    std::string dummy;
    int skipped = 0;

    /** 
     * Do not increment line_num_ here — next_row() will do it correctly
     * Using skipped counter instead of line_num_ to track skipped lines, since line_num_ handled later
    **/   
    while (skipped < static_cast<size_t>(start_row - 1) && std::getline(file_, dummy)) 
    {
        // ++line_num_; 
        ++skipped;
        std::cerr << "[DBG] skipped row " << skipped << ": '" << dummy << "'\n";
    }
    std::cerr << "\n[DBG] skip phase done -- skipped=" << skipped << "\n-----------------------------\n";

    // ── Phase 3: parse rows into table ───────────────────────────────────────
    while (auto row = next_row(start_col)) {
        // std::cerr << "[DBG] next_row -> line=" << row->line_number
        //           << " fields=" << row->fields.size()
        //           << " first='" << (!row->fields.empty() ? row->fields[0] : "<empty>")
        //           << "'\t";

        if (!row->fields.empty()) {
            table[static_cast<int>(row->line_number)] = std::move(row->fields);
            // std::cerr << "[DBG] added at key=" << row->line_number << "\n";
        } else {
            std::cerr << "[WARN] row " << row->line_number << " has 0 fields — skipped\n";
        }
    }

    // ── Phase 4: summary ─────────────────────────────────────────────────────
    std::cerr << "\n[DBG] parse_all complete -- table size=" << table.size() << "\n-----------------------------\n";
    
    // for (const auto& [k, v] : table)
    //     std::cerr << "[DBG] table[" << k << "] = " << v.size() << " fields" << " | first='" << (!v.empty() ? v[0] : "<empty>") << "'\n";

    if (table.empty())
        throw std::runtime_error("No data found from row " + std::to_string(start_row) + ", col " + std::to_string(start_col));
    return table;
}

void CSVParser::export_csv(const std::map<int, std::vector<std::string>> &table, const std::string &filename) const
{
    std::ofstream out(filename);
    if (!out.is_open())
        throw std::runtime_error("Cannot open output file: " + filename);

    for (const auto& [row_num, fields] : table) {
        for (size_t i = 0; i < fields.size(); ++i) {
            const std::string& field = fields[i];

            // Re-quote fields that contain delimiter, quotes, or newlines
            bool needs_quotes = field.find(delimiter_) != std::string::npos ||
                                field.find('"')        != std::string::npos ||
                                field.find('\n')       != std::string::npos;

            if (needs_quotes) {
                std::string escaped = field;
                size_t pos = 0;
                while ((pos = escaped.find('"', pos)) != std::string::npos) {
                    escaped.insert(pos, 1, '"'); // escape " → ""
                    pos += 2;
                }
                out << '"' << escaped << '"';
            } else {
                out << field;
            }

            if (i < fields.size() - 1) out << delimiter_;
        }
        out << '\n';
    }

    if (out.fail())
        throw std::runtime_error("Write error on file: " + filename);

    std::cout << "[DEBUG] Exported " << table.size() << " rows to: " << filename << "\n";
}

std::optional<dataformat::Row> CSVParser::next_row(int start_col) {
    std::string line;
    while (std::getline(file_, line)) {
        ++line_num_;
        if (!line.empty() && line.back() == '\r') line.pop_back(); // strip CRLF
        if (line.empty()) continue; // blank line — skip, don't return

        dataformat::Row row;
        row.line_number = line_num_;
        row.fields = parse_line(line, start_col);
        return row; // return immediately — avoids stale line value after EOF
    }
    return std::nullopt; // only reached at true EOF
}

// RFC 4180 state machine: handles quotes, embedded delimiters, escaped quotes
std::vector<std::string> CSVParser::parse_line(const std::string& line, int start_col) {
    std::vector<std::string> fields;
    std::string field;
    bool in_quotes = false;
    int col = 1; // Track column index for start_col, 1-based
    
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
