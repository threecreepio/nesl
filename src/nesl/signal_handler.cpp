#include "signal_handler.h"

#include <cstdio>

#ifdef __linux__

#include <csignal>
#include <cstring>
#include <execinfo.h>
#include <unistd.h>

namespace {

constexpr int kBacktraceDepth = 32;

const char* signal_name(int sig) {
    switch (sig) {
        case SIGSEGV: return "SIGSEGV";
        case SIGABRT: return "SIGABRT";
        case SIGFPE:  return "SIGFPE";
        default:      return nullptr;
    }
}

void write_str(int fd, const char* s, size_t n) {
    // Best-effort write; in a signal handler we cannot meaningfully handle
    // a short write or EINTR, so just call write() once.
    while (n > 0) {
        ssize_t w = ::write(fd, s, n);
        if (w <= 0) return;
        s += w;
        n -= w;
    }
}

void write_backtrace(int sig) {
    // Async-signal-safe only: no fprintf/fflush (they take stdio locks that
    // may be held by the crashing thread). Use raw write() for the header
    // line; backtrace_symbols_fd() is the documented safe primitive for
    // emitting the actual backtrace.
    char buf[128];
    const char* name = signal_name(sig);
    int n;
    if (name != nullptr) {
        n = snprintf(buf, sizeof(buf), "\nnesl: caught signal %d (%s)\n",
                     sig, name);
    } else {
        n = snprintf(buf, sizeof(buf), "\nnesl: caught signal %d\n", sig);
    }
    if (n > 0) write_str(STDERR_FILENO, buf, (size_t)n);

    void* frames[kBacktraceDepth];
    int nframes = backtrace(frames, kBacktraceDepth);
    n = snprintf(buf, sizeof(buf), "nesl: backtrace (%d frames):\n", nframes);
    if (n > 0) write_str(STDERR_FILENO, buf, (size_t)n);
    backtrace_symbols_fd(frames, nframes, STDERR_FILENO);
}

extern "C" void nesl_signal_handler(int sig) {
    write_backtrace(sig);

    // Restore the default disposition and re-raise so the process exits
    // with the canonical status for this signal (e.g. 139 for SIGSEGV,
    // 134 for SIGABRT) rather than returning from the handler.
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(sig, &sa, nullptr);
    raise(sig);
}

}  // namespace

void nesl_install_signal_handlers(void) {
    struct sigaction sa;
    sa.sa_handler = nesl_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);
}

#else  // !__linux__

void nesl_install_signal_handlers(void) {
}

#endif  // __linux__
