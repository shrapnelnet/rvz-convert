#include <getopt.h>
#include <iostream>
#define VERSION "v1.0.0-rc2"

void version() {
    std::cout << "rvz-convert " << VERSION << std::endl;
    std::cout << "Copyright © 2025 <github/shrapnelnet>" << std::endl;
    std::cout << "License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>" << std::endl;
    std::cout << "This is free software: you are free to change and redistribute it." << std::endl;
    std::cout << "There is NO WARRANTY, to the extent permitted by law." << std::endl;
}

void help() {
    std::cout << "Usage: rvz-convert [options] file.rvz" << std::endl;
    std::cout << "Options:\n\t-h --help\tDisplay this message" << std::endl;
    std::cout << "\t-v --version\tDisplay version information" << std::endl;
}

void parseOptions(int argc, char *argv[]) {
    int c;
    int option_index = 0;

    while (true) {
        static option options[] = {
            {"version",no_argument, nullptr, 'v'},
            {"help", no_argument, nullptr, 'h'},
            {nullptr, 0, nullptr, 0}
        };
        c = getopt_long(argc, argv, "hv", options, &option_index);
        if (c == -1)
            break;
        switch (c) {
            case 'h':
                help();
                break;
            case 'v':
                version();
                break;
            default:
                help();
                break;
        }
    }

    if (optind == argc) {
        help();
        exit(0);
    }
}
