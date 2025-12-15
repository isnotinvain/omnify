#include "DaemonManager.h"

#include <juce_core/juce_core.h>

#include "OmnifyLogger.h"

// Socket headers still needed for findFreePort()
#if JUCE_MAC || JUCE_LINUX
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#elif JUCE_WINDOWS
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#if JUCE_MAC || JUCE_LINUX
#if !JUCE_DEBUG
#include <dlfcn.h>
#endif
#include <cstdlib>  // for confstr on macOS
#endif

DaemonManager::DaemonManager() : juce::Thread("DaemonOutputReader"), process(std::make_unique<juce::ChildProcess>()) {}

void DaemonManager::setListener(Listener* l) { listener = l; }

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
    // Use launcher script to redirect stdout/stderr to log file
    // This avoids pipe buffer blocking issues with ChildProcess
    juce::StringArray args;
    auto tempDir = OmnifyLogger::getTempDir().getFullPathName();

#if JUCE_DEBUG
    // Development: use uv run from project root
    auto projectRoot = getProjectRoot();
    auto launcherScript = projectRoot.getChildFile("vst/Resources/launch_daemon.sh");

    args.add(tempDir);
    args.add(juce::String(oscPort));
    args.add("uv");
    args.add("run");
    args.add("python");
    args.add("-m");
    args.add("daemomnify");
    args.add("--osc-port");
    args.add(juce::String(oscPort));
    args.add("--temp-dir");
    args.add(tempDir);

    return {launcherScript.getFullPathName(), args};
#else
    // Release: use bundled executable
    // Use dladdr to find the path to our own shared library (the VST3 plugin)
    Dl_info dlInfo;
    juce::File pluginBundle;
    // Use address of this function (converted via reinterpret_cast for dladdr compatibility)
    static int dummy = 0;
    if (dladdr(&dummy, &dlInfo) && dlInfo.dli_fname) {
        OmnifyLogger::log("dladdr succeeded, dli_fname: " + juce::String(dlInfo.dli_fname));
        // dli_fname gives us the path to the .so/.dylib inside MacOS/
        // Go up to Contents, then to the bundle root
        pluginBundle = juce::File(dlInfo.dli_fname).getParentDirectory().getParentDirectory().getParentDirectory();
        OmnifyLogger::log("pluginBundle: " + pluginBundle.getFullPathName());
    } else {
        OmnifyLogger::log("dladdr FAILED");
    }
    auto resourcesDir = pluginBundle.getChildFile("Contents/Resources");
    auto launcherScript = resourcesDir.getChildFile("launch_daemon.sh");
    auto daemonExe = resourcesDir.getChildFile("daemomnify");
    OmnifyLogger::log("resourcesDir: " + resourcesDir.getFullPathName());
    OmnifyLogger::log("launcherScript exists: " + juce::String(launcherScript.existsAsFile() ? "yes" : "no"));
    OmnifyLogger::log("daemonExe exists: " + juce::String(daemonExe.existsAsFile() ? "yes" : "no"));

    args.add(tempDir);
    args.add(juce::String(oscPort));
    args.add(daemonExe.getFullPathName());
    args.add("--osc-port");
    args.add(juce::String(oscPort));
    args.add("--temp-dir");
    args.add(tempDir);

    return {launcherScript.getFullPathName(), args};
#endif
}

bool DaemonManager::start() {
    OmnifyLogger::log("DaemonManager::start() called");

    if (isRunning()) {
        OmnifyLogger::log("Already running, returning");
        return true;
    }

    // Find a free port for OSC
    oscPort = findFreePort();
    if (oscPort == 0) {
        OmnifyLogger::log("Failed to find free port");
        return false;
    }

    OmnifyLogger::log("Found port: " + juce::String(oscPort));

    // Delete any stale ready file before starting
    getReadyFile().deleteFile();
    readyFired = false;

    auto [command, args] = getLaunchCommand();

    // Build full command string for ChildProcess
    juce::String fullCommand = command;
    for (const auto& arg : args) {
        fullCommand += " " + arg;
    }

    OmnifyLogger::log("Command: " + fullCommand);
    OmnifyLogger::log("About to call process->start()...");

    // Don't capture stdout/stderr - launcher script redirects to log file
    if (!process->start(fullCommand, 0)) {
        OmnifyLogger::log("process->start() returned false");
        return false;
    }

    OmnifyLogger::log("process->start() returned true");
    OmnifyLogger::log("process->isRunning(): " + juce::String(process->isRunning() ? "yes" : "no"));

    // Start the output reader thread
    startThread();
    OmnifyLogger::log("Started output reader thread");

    // Connect OSC sender to the daemon
    if (!oscSender.connect("127.0.0.1", oscPort)) {
        OmnifyLogger::log("OSC connect failed");
    } else {
        OmnifyLogger::log("OSC connected to port " + juce::String(oscPort));
    }

    OmnifyLogger::log("DaemonManager::start() completed successfully");
    return true;
}

void DaemonManager::run() {
    // Poll for ready file to trigger daemonReady callback
    // Daemon stdout/stderr is redirected to log file by launch_daemon.sh
    auto readyFile = getReadyFile();

    while (!threadShouldExit() && process && process->isRunning()) {
        // Check for ready file if we haven't fired yet
        if (!readyFired && listener && readyFile.existsAsFile()) {
            readyFired = true;
            DBG("DaemonManager: Ready file detected");
            // Call listener on message thread to avoid threading issues
            juce::MessageManager::callAsync([this]() {
                if (listener) {
                    listener->daemonReady();
                }
            });
        }

        // Sleep briefly to avoid spinning CPU
        Thread::sleep(50);
    }
}

void DaemonManager::sendOscQuit() { oscSender.send("/quit"); }

juce::File DaemonManager::getReadyFile() const { return OmnifyLogger::getTempDir().getChildFile("daemomnify-" + juce::String(oscPort) + ".ready"); }

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