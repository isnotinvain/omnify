#pragma once

#include <juce_core/juce_core.h>
#include <juce_osc/juce_osc.h>

/**
 * Manages the lifecycle of the Python daemon process.
 *
 * Each VST instance spawns its own daemon on a unique port.
 * - Debug: runs "uv run python -m daemomnify --osc-port <port>"
 * - Release: runs bundled executable with "--osc-port <port>"
 *
 * The daemon handles its own crash recovery via __main__.py's restart loop.
 * Daemon stdout/stderr is forwarded to the host app's stdout/stderr.
 */
class DaemonManager : private juce::Thread {
   public:
    /** Callback interface for daemon events. */
    class Listener {
       public:
        virtual ~Listener() = default;
        /** Called when the daemon's OSC server is ready to receive messages. */
        virtual void daemonReady() = 0;
    };

    DaemonManager();
    ~DaemonManager() override;

    // Non-copyable
    DaemonManager(const DaemonManager&) = delete;
    DaemonManager& operator=(const DaemonManager&) = delete;

    /** Set the listener for daemon events. */
    void setListener(Listener* l);

    /** Start the daemon process. Returns true if successful. */
    bool start();

    /** Stop the daemon process. */
    void stop();

    /** Check if daemon is currently running. */
    bool isRunning() const;

    /** Get the OSC port this daemon is listening on. */
    int getPort() const { return oscPort; }

    /** Get the OSC sender for sending messages to daemon. */
    juce::OSCSender& getOscSender() { return oscSender; }

   private:
    /** Thread run loop - forwards daemon stdout/stderr. */
    void run() override;

    /** Find a free TCP/UDP port by binding to port 0. */
    static int findFreePort();

    /** Get the path to the project root (for debug builds). */
    static juce::File getProjectRoot();

    /** Get the command and arguments to launch the daemon. */
    std::pair<juce::String, juce::StringArray> getLaunchCommand() const;

    /** Send OSC /quit message to daemon for graceful shutdown. */
    void sendOscQuit();

    static constexpr const char* READY_MARKER = "<DAEMOMNIFY_OSC_SERVER_READY>";

    std::unique_ptr<juce::ChildProcess> process;
    juce::OSCSender oscSender;
    Listener* listener = nullptr;
    int oscPort = 0;
};