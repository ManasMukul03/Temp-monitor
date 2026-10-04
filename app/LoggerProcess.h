/*
 * LoggerProcess.h - writes the log file from a SEPARATE PROCESS.
 *
 *   start(): pipe() creates a one-way channel, fork() creates a child.
 *            The child reads lines from the pipe and appends them to the file.
 *   log():   the parent writes a line into the pipe (fast, never blocks on disk).
 *   stop():  the parent closes the pipe; the child reads end-of-file,
 *            writes a final line and exits; the parent waits for it (waitpid).
 */
#ifndef LOGGERPROCESS_H
#define LOGGERPROCESS_H

#include <mutex>
#include <string>
#include <sys/types.h>

class LoggerProcess {
public:
    ~LoggerProcess();

    bool start(const std::string& logPath);
    void log(const std::string& message);
    void stop();

    bool isRunning() const { return childPid_ > 0; }
    pid_t childPid() const { return childPid_; }

private:
    std::mutex mutex_;       // the monitor thread and the menu may both log
    pid_t childPid_ = -1;
    int writeFd_ = -1;       // parent's end of the pipe
};

#endif
