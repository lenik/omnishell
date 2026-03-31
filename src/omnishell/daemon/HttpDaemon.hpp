#ifndef HTTPDAEMON_H
#define HTTPDAEMON_H

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

struct ssl_ctx_st;
typedef struct ssl_ctx_st SSL_CTX;

class VolumeManager;
class VolumeIndex;
class ApiVolumeIndex;
class DirIndex;
class ApiDirIndex;
class FileController;
class ApiFileController;
class AssetController;

/**
 * Lightweight HTTP/HTTPS daemon for bridging VFS with web interface
 * Separated from wxWidgets for headless operation
 */
class HttpDaemon {
private:
    VolumeManager* m_volumeManager;
    std::string m_bindAddress;
    int m_httpPort;
    int m_httpsPort;
    std::atomic<bool> m_running;
    std::vector<std::thread> m_serverThreads;

    // Listening sockets for serverLoop() (used to interrupt blocking accept() on stop()).
    std::atomic<int> m_httpListenSocket{-1};
    std::atomic<int> m_httpsListenSocket{-1};
    
    std::unique_ptr<VolumeIndex> m_volumeIndex;
    std::unique_ptr<ApiVolumeIndex> m_apiVolumeIndex;
    std::unique_ptr<DirIndex> m_dirIndex;
    std::unique_ptr<ApiDirIndex> m_apiDirIndex;
    std::unique_ptr<FileController> m_fileController;
    std::unique_ptr<ApiFileController> m_apiFileController;
    std::unique_ptr<AssetController> m_assetController;
    SSL_CTX* m_ssl_ctx = nullptr;             // For HTTPS
    
    bool initHttps();
    void shutdownHttps();
    void serverLoop(int port, bool useHttps);
    void handleRequest(int clientSocket, bool useHttps);
    std::string generateResponse(const std::string& method, const std::string& path, const std::string& body);
    std::string routeRequest(const std::string& method, const std::string& path, const std::string& body);
    
public:
    HttpDaemon(VolumeManager* volumeManager, 
               const std::string& bindAddress = "127.0.0.1", 
               int httpPort = 4380, 
               int httpsPort = 4381);
    ~HttpDaemon();
    
    // Server control (HTTP always; HTTPS only when @p withHttps is true)
    bool start(bool withHttps = true);
    void stop();
    bool isRunning() const { return m_running; }
    
    // Configuration
    void setBindAddress(const std::string& address) { m_bindAddress = address; }
    void setHttpPort(int port) { m_httpPort = port; }
    void setHttpsPort(int port) { m_httpsPort = port; }
    int getHttpPort() const { return m_httpPort; }
    int getHttpsPort() const { return m_httpsPort; }
    const std::string& getBindAddress() const { return m_bindAddress; }
};

#endif // HTTPDAEMON_H
