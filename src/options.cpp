#include "options.h"

#include <cstdlib>
#include <getopt.h>
#include <iostream>
#define VERSION "v1.1.0"

void version() {
    std::cout << "rvz-convert " << VERSION << std::endl;
    std::cout << "Copyright © 2025 <github/shrapnelnet>" << std::endl;
    std::cout << "License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>" << std::endl;
    std::cout << "This is free software: you are free to change and redistribute it." << std::endl;
    std::cout << "There is NO WARRANTY, to the extent permitted by law." << std::endl;
}

void help() {
    std::cout << "Usage: rvz-convert [options] file.rvz [file2.rvz ...]" << std::endl;
    std::cout << "Options:\n\t-h --help\tDisplay this message" << std::endl;
    std::cout << "\t-v --version\tDisplay version information" << std::endl;
    std::cout << "\t-j --jobs N\tConvert up to N games at the same time (default: 1)" << std::endl;
    std::cout << "\t-k --keep-logs\tKeep the conversion log even when it succeeds" << std::endl;
}

Options parseOptions(int argc, char *argv[]) {
    Options opts;
    int c;
    int option_index = 0;

    while (true) {
        static option options[] = {
            {"version", no_argument, nullptr, 'v'},
            {"help", no_argument, nullptr, 'h'},
            {"jobs", required_argument, nullptr, 'j'},
            {"keep-logs", no_argument, nullptr, 'k'},
            {nullptr, 0, nullptr, 0}
        };
        c = getopt_long(argc, argv, "hvj:k", options, &option_index);
        if (c == -1)
            break;
        switch (c) {
            case 'h':
                help();
                exit(0);
            case 'v':
                version();
                exit(0);
            case 'j': {
                char *end = nullptr;
                long jobs = std::strtol(optarg, &end, 10);
                if (*end != '\0' || jobs < 1) {
                    std::cerr << "Error: --jobs expects a positive number, got: " << optarg << std::endl;
                    exit(2);
                }
                opts.jobs = static_cast<unsigned int>(jobs);
                break;
            }
            case 'k':
                opts.keepLogs = true;
                break;
            default:
                help();
                exit(2);
        }
    }

    if (optind == argc) {
        help();
        exit(0);
    }

    for (int i = optind; i < argc; i++) {
        opts.files.emplace_back(argv[i]);
    }
    return opts;
}
