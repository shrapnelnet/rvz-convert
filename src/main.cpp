// greetZ dolphin-emu and WIT team!
#include <algorithm>

#include "options.h"
#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

// serialises terminal output so lines from parallel jobs don't get mixed
std::mutex outputMutex;

void logInfo(const std::string& game, const std::string& message) {
    std::lock_guard lock(outputMutex);
    std::cout << "[" << game << "] " << message << std::endl;
}

void logError(const std::string& game, const std::string& message) {
    std::lock_guard lock(outputMutex);
    std::cerr << "[" << game << "] Error: " << message << std::endl;
}

std::string readFile(const fs::path& path) {
    std::ifstream file(path);
    std::stringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

std::string joinArgs(const std::vector<std::string>& args) {
    std::string line;
    for (const auto& arg : args) {
        if (!line.empty()) line += ' ';
        line += arg.find(' ') == std::string::npos ? arg : "\"" + arg + "\"";
    }
    return line;
}

// runs a program directly (no shell, so file names never need escaping).
// stdout and stderr go to logFd, unless captured is set, in which case
// stdout is returned through it instead. returns the exit code.
int runCommand(const std::vector<std::string>& args, int logFd, std::string *captured = nullptr) {
    std::string header = "$ " + joinArgs(args) + "\n";
    if (write(logFd, header.data(), header.size()) < 0) {
        return -1;
    }

    std::vector<char *> argv;
    for (const auto& arg : args) {
        argv.push_back(const_cast<char *>(arg.c_str()));
    }
    argv.push_back(nullptr);

    int pipeFds[2] = {-1, -1};
    if (captured && pipe2(pipeFds, O_CLOEXEC) != 0) {
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        return -1;
    }
    if (pid == 0) {
        dup2(captured ? pipeFds[1] : logFd, STDOUT_FILENO);
        dup2(logFd, STDERR_FILENO);
        if (captured) {
            close(pipeFds[0]);
            close(pipeFds[1]);
        }
        execvp(argv[0], argv.data());
        dprintf(STDERR_FILENO, "could not run %s: %s\n", argv[0], strerror(errno));
        _exit(127);
    }

    if (captured) {
        close(pipeFds[1]);
        char buf[4096];
        ssize_t n;
        while ((n = read(pipeFds[0], buf, sizeof(buf))) > 0) {
            captured->append(buf, n);
        }
        close(pipeFds[0]);
    }

    int status = 0;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return -1;
}

// converts one game, returns true on success.
// every run gets its own temporary directory, so several conversions
// (in this process or in separate ones) can share the same folder.
bool convertGame(const std::string& filepath, bool keepLogs) {
    // extract filename without extension
    std::string filename = fs::path(filepath).stem().string();

    if (!fs::is_regular_file(filepath)) {
        logError(filename, "File not found: " + filepath);
        return false;
    }

    std::string tmpTemplate = "./.rvz-convert-XXXXXX";
    if (!mkdtemp(tmpTemplate.data())) {
        logError(filename, std::format("Could not create temporary directory: {}", strerror(errno)));
        return false;
    }
    fs::path tmpDir = tmpTemplate;
    fs::path tmpIso = tmpDir / "tmp.iso";
    fs::path logPath = tmpDir / "convert.log";
    int logFd = open(logPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (logFd < 0) {
        logError(filename, std::format("Could not create log file: {}", strerror(errno)));
        fs::remove_all(tmpDir);
        return false;
    }

    fs::path outDir;
    bool createdOutDir = false;

    // prints the tool output and keeps the log next to the game for later
    auto fail = [&](const std::string& message) {
        close(logFd);
        fs::path savedLog = filename + ".rvz-convert.log";
        std::error_code ec;
        fs::copy_file(logPath, savedLog, fs::copy_options::overwrite_existing, ec);
        {
            std::lock_guard lock(outputMutex);
            std::cerr << "[" << filename << "] Error: " << message << std::endl;
            std::cerr << "[" << filename << "] ----- tool output -----" << std::endl;
            std::istringstream lines(readFile(logPath));
            for (std::string line; std::getline(lines, line);) {
                std::cerr << "[" << filename << "] " << line << std::endl;
            }
            std::cerr << "[" << filename << "] -----------------------" << std::endl;
            if (!ec) {
                std::cerr << "[" << filename << "] Log saved to: " << savedLog.string() << std::endl;
            }
        }
        if (createdOutDir) {
            fs::remove_all(outDir, ec);
        }
        fs::remove_all(tmpDir, ec);
        return false;
    };

    // convert rvz to iso using dolphin engine
    logInfo(filename, "Converting RVZ to ISO...");
    int code = runCommand({"dolphin-tool", "convert", "-i", filepath, "-f", "iso", "-o", tmpIso.string()}, logFd);
    if (code != 0) {
        return fail(std::format("dolphin-tool failed (exit code {})", code));
    }
    if (!fs::exists(tmpIso)) {
        return fail("dolphin-tool did not produce an ISO");
    }

    // get game ID using wit (required for USB loader GX)
    std::string id;
    code = runCommand({"wit", "id", tmpIso.string()}, logFd, &id);
    id.erase(std::remove_if(id.begin(), id.end(), ::isspace), id.end());
    if (code != 0) {
        return fail(std::format("wit id failed (exit code {})", code));
    }
    if (id.empty()) {
        return fail("wit id did not return a game ID");
    }

    // create folder to put WBFS in
    outDir = std::format("{} [{}]", filename, id);
    fs::path wbfs = outDir / std::format("[{}].wbfs", id);
    if (fs::exists(wbfs)) {
        return fail("Output already exists: " + wbfs.string());
    }
    std::error_code ec;
    createdOutDir = fs::create_directory(outDir, ec);
    if (ec) {
        return fail(std::format("Could not create folder {}: {}", outDir.string(), ec.message()));
    }

    // convert iso to WBFS using wit
    logInfo(filename, "Converting ISO to WBFS...");
    code = runCommand({"wit", "cp", tmpIso.string(), wbfs.string()}, logFd);
    if (code != 0) {
        return fail(std::format("wit cp failed (exit code {})", code));
    }

    close(logFd);
    if (keepLogs) {
        fs::copy_file(logPath, outDir / "rvz-convert.log", fs::copy_options::overwrite_existing, ec);
    }
    // remove temporary iso
    fs::remove_all(tmpDir, ec);

    logInfo(filename, "Done: " + wbfs.string());
    return true;
}

int main(const int argc, char *argv[]) {
    // get runtime flags/filepaths
    Options opts = parseOptions(argc, argv);

    std::vector<std::string> failed;
    std::mutex failedMutex;
    std::atomic<size_t> next = 0;

    auto worker = [&] {
        for (size_t i; (i = next++) < opts.files.size();) {
            if (!convertGame(opts.files[i], opts.keepLogs)) {
                std::lock_guard lock(failedMutex);
                failed.push_back(opts.files[i]);
            }
        }
    };

    unsigned int jobs = std::min<size_t>(opts.jobs, opts.files.size());
    std::vector<std::thread> threads;
    for (unsigned int i = 1; i < jobs; i++) {
        threads.emplace_back(worker);
    }
    worker();
    for (auto& t : threads) {
        t.join();
    }

    if (opts.files.size() > 1) {
        std::cout << std::format("{} of {} games converted.", opts.files.size() - failed.size(), opts.files.size())
                  << std::endl;
    }
    if (!failed.empty()) {
        std::cerr << "Failed:" << std::endl;
        for (const auto& f : failed) {
            std::cerr << "  " << f << std::endl;
        }
        return 1;
    }
    return 0;
}
