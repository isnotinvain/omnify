#include "DaemonManager.h"

#include <juce_core/juce_core.h>

#include <iostream>

// Socket headers still needed for findFreePort()
#if JUCE_MAC || JUCE_LINUX
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#elif JUCE_WINDOWS
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif

DaemonManager::DaemonManager() : juce::Thread("DaemonOutputReader"), process(std::make_unique<juce::ChildProcess>()) {}

DaemonManager::~DaemonManager() {
    stop();
    stopThread(2000);
}

int DaemonManager::findFreePort() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return 0;
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;  // Let OS assign a free port

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
#if JUCE_MAC || JUCE_LINUX
        close(sock);
#elif JUCE_WINDOWS
        closesocket(sock);
#endif
        return 0;
    }

    socklen_t addrLen = sizeof(addr);
    if (getsockname(sock, (struct sockaddr*)&addr, &addrLen) < 0) {
#if JUCE_MAC || JUCE_LINUX
        close(sock);
#elif JUCE_WINDOWS
        closesocket(sock);
#endif
        return 0;
    }

    int port = ntohs(addr.sin_port);

#if JUCE_MAC || JUCE_LINUX
    close(sock);
#elif JUCE_WINDOWS
    closesocket(sock);
#endif

    return port;
}

juce::File DaemonManager::getProjectRoot() {
    // In debug builds, __FILE__ is in vst/, so go up one level
    return juce::File(__FILE__).getParentDirectory().getParentDirectory();
}

std::pair<juce::String, juce::StringArray> DaemonManager::getLaunchCommand() const {
#if JUCE_DEBUG
    // Development: use uv run from project root
    auto projectRoot = getProjectRoot();
    juce::StringArray args;
    args.add("run");
    args.add("python");
    args.add("-m");
    args.add("daemomnify");
    args.add("--osc-port");
    args.add(juce::String(oscPort));

    return {"uv", args};
#else
    // Release: use bundled executable
    // Look for it relative to the plugin binary
    auto pluginFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    auto resourcesDir = pluginFile.getParentDirectory().getParentDirectory().getChildFile("Resources");
    auto daemonExe = resourcesDir.getChildFile("daemomnify");

    juce::StringArray args;
    args.add("--osc-port");
    args.add(juce::String(oscPort));

    return {daemonExe.getFullPathName(), args};
#endif
}

bool DaemonManager::start() {
    if (isRunning()) {
        return true;
    }

    // Find a free port for OSC
    oscPort = findFreePort();
    if (oscPort == 0) {
        DBG("DaemonManager: Failed to find free port");
        return false;
    }

    DBG("DaemonManager: Starting daemon on OSC port " << oscPort);

    auto [command, args] = getLaunchCommand();

    // Build full command string for ChildProcess
    juce::String fullCommand = command;
    for (const auto& arg : args) {
        fullCommand += " " + arg;
    }

#if JUCE_DEBUG
    // Set working directory to project root for uv to find pyproject.toml
    auto projectRoot = getProjectRoot();
    DBG("DaemonManager: Working directory: " << projectRoot.getFullPathName());
    DBG("DaemonManager: Running: " << fullCommand);

    // Use start() with working directory
    if (!process->start(fullCommand, juce::ChildProcess::wantStdOut | juce::ChildProcess::wantStdErr)) {
        DBG("DaemonManager: Failed to start process");
        return false;
    }
#else
    if (!process->start(fullCommand)) {
        DBG("DaemonManager: Failed to start process");
        return false;
    }
#endif

    // Start the output reader thread
    startThread();

    // Connect OSC sender to the daemon
    if (!oscSender.connect("127.0.0.1", oscPort)) {
        DBG("DaemonManager: Failed to connect OSC sender");
    }

    DBG("DaemonManager: Daemon started successfully");
    return true;
}

void DaemonManager::run() {
    // Read and forward daemon output (stdout + stderr interleaved) to stdout
    while (!threadShouldExit() && process && process->isRunning()) {
        char buffer[1024];
        auto bytesRead = process->readProcessOutput(buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            std::cout << buffer << std::flush;
        } else {
            // No data available, sleep briefly
            Thread::sleep(10);
        }
    }
}

void DaemonManager::sendOscQuit() {
    oscSender.send("/quit");
}

void DaemonManager::stop() {
    if (!process || !process->isRunning()) {
        return;
    }

    DBG("DaemonManager: Stopping daemon");

    // Send OSC /quit for graceful shutdown
    sendOscQuit();

    // Wait up to 2 seconds for graceful exit
    if (process->waitForProcessToFinish(2000)) {
        DBG("DaemonManager: Daemon exited gracefully");
        return;
    }

    // Force kill if graceful shutdown didn't work
    DBG("DaemonManager: Forcing kill");
    process->kill();
    process->waitForProcessToFinish(1000);
}

bool DaemonManager::isRunning() const { return process && process->isRunning(); }