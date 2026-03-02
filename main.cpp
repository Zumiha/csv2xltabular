#include "csv2xltabular.h"

int main(int argc, char* argv[]) {
    try {
        CSVtoXLTABularConverter converter("data.csv", "settings.ini");
        converter.convert();
        converter.exportToFile();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}