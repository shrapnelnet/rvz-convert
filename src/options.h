#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>
#include <vector>

struct Options {
    // number of games converted at the same time
    unsigned int jobs = 1;
    // keep the full tool log even when the conversion succeeds
    bool keepLogs = false;
    std::vector<std::string> files;
};

void help();
void version();
Options parseOptions(int argc, char *argv[]);

#endif //OPTIONS_H
