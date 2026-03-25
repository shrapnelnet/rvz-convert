// greetZ dolphin-emu and WIT team!
#include <algorithm>

#include "options.h"
#include <format>
#include <cstdlib>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>

bool fileExists(const std::string& filepath) {
    std::ifstream file(filepath);
    return file.good();
}

int main(const int argc, char *argv[]) {
    // get runtime flags/filepath
    parseOptions(argc, argv);
    char *filepath = argv[argc - 1];

    // check if file exists
    if (!fileExists(filepath)) {
        std::cerr << "Error: File not found: " << filepath << std::endl;
        return 1;
    }

    // convert rvz to iso using dolphin engine
    auto dolphinCommand =  std::format("dolphin-tool convert -i \"{}\" -f iso -o tmp.iso", filepath);
    std::system(dolphinCommand.c_str());

    // get game ID using wit (required for USB loader GX)
    std::system("wit id tmp.iso > id.txt");
    auto idstream = std::ifstream("id.txt");
    std::stringstream filebuf;
    filebuf << idstream.rdbuf();
    std::string id = filebuf.str();
    std::system("rm id.txt");
    id.erase(std::remove_if(id.begin(), id.end(), ::isspace), id.end());

    // extract filename without extension
    std::string filename(filepath);
    size_t lastSlash = filename.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        filename = filename.substr(lastSlash + 1);
    }
    size_t lastDot = filename.find_last_of(".");
    if (lastDot != std::string::npos) {
        filename = filename.substr(0, lastDot);
    }

    // create folder to put WBFS in
    auto folderCommand = std::format("mkdir \"{} [{}]\"", filename, id);
    std::system(folderCommand.c_str());

    // convert iso to WBFS using wit
    auto witCommand = std::format("wit cp tmp.iso \"{} [{}]\"/[{}].wbfs", filename, id, id);
    std::system(witCommand.c_str());

    // remove temporary iso
    std::system("rm tmp.iso");

    return 0;
}
