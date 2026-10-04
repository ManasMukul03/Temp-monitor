#include "LoggerProcess.h"
#include "Types.h"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <unistd.h>      // pipe, fork, read, write, close
#include <sys/wait.h>    // waitpid

LoggerProcess::~LoggerProcess() { stop(); }

// Code that runs only in the child process
static void childLoop(int readFd, const std::string& path) {
    std::signal(SIGINT, SIG_IGN);     // Ctrl+C is handled by the parent, which then stops us cleanly
    FILE* file = std::fopen(path.c_str(), "a");
    if (!file) _exit(1);

    char buf[512];
    while (true) {
        ssize_t n = ::read(readFd, buf, sizeof(buf));
        if (n > 0) {
            std::fwrite(buf, 1, n, file);
            std::fflush(file);                 // write to disk immediately
        } else if (n == 0) {
            break;                             // parent closed the pipe -> end of file
        } else if (errno != EINTR) {
            break;
        }
    }
    std::fprintf(file, "%s LOGGER process %d stopped\n",
                 formatTime(Clock::now()).c_str(), getpid());
    std::fclose(file);
    ::close(readFd);
    _exit(0);                                  // end the child without running parent's cleanup
}

bool LoggerProcess::start(const std::string& logPath) {
    if (childPid_ > 0) return true;

    int fds[2];                                // fds[0] = read end, fds[1] = write end
    if (::pipe(fds) < 0) return false;

    pid_t pid = ::fork();                      // from here, two processes run this code
    if (pid < 0) {
        ::close(fds[0]);
        ::close(fds[1]);
        return false;
    }
    if (pid == 0) {                            // child
        ::close(fds[1]);
        childLoop(fds[0], logPath);
    }
    // parent
    ::close(fds[0]);
    writeFd_ = fds[1];
    childPid_ = pid;
    log("LOGGER process " + std::to_string(pid) + " started, writing to " + logPath);
    return true;
}

void LoggerProcess::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (writeFd_ < 0) return;
    std::string line = formatTime(Clock::now()) + " " + message + "\n";
    const char* p = line.data();
    size_t left = line.size();
    while (left > 0) {
        ssize_t n = ::write(writeFd_, p, left);
        if (n < 0) {
            if (errno == EINTR) continue;
            return;                            // child gone; ignore
        }
        p += n;
        left -= n;
    }
}

void LoggerProcess::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (writeFd_ >= 0) {
        ::close(writeFd_);                     // child sees end-of-file
        writeFd_ = -1;
    }
    if (childPid_ > 0) {
        ::waitpid(childPid_, nullptr, 0);      // wait so no zombie process is left
        childPid_ = -1;
    }
}
