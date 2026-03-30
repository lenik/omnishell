#include "HttpDaemon.hpp"

#include "ApiDirIndex.hpp"
#include "ApiFileController.hpp"
#include "ApiVolumeIndex.hpp"
#include "AssetController.hpp"
#include "DirIndex.hpp"
#include "FileController.hpp"
#include "VolumeIndex.hpp"

#include <bas/proc/AssetsRegistry.hpp>
#include <bas/volume/VolumeManager.hpp>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string_view>

#include <netinet/in.h>

#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

#include <arpa/inet.h>

#include <sys/socket.h>

namespace {

std::string stripHttpRequestQueryAndFragment(std::string_view p) {
    for (size_t i = 0; i < p.size(); ++i) {
        if (p[i] == '?' || p[i] == '#')
            return std::string(p.substr(0, i));
    }
    return std::string(p);
}

int hexDigitVal(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

/** Decode %HH sequences in the request path (UTF-8 file names from browsers). */
std::string percentDecodeHttpPath(const std::string& rawPath) {
    std::string path = stripHttpRequestQueryAndFragment(rawPath);
    std::string out;
    out.reserve(path.size());
    for (size_t i = 0; i < path.size(); ++i) {
        if (path[i] == '%' && i + 2 < path.size()) {
            int hi = hexDigitVal(path[i + 1]);
            int lo = hexDigitVal(path[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
                continue;
            }
        }
        out.push_back(path[i]);
    }
    return out;
}

} // namespace

#include <fcntl.h>
#include <unistd.h>

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/evp.h>
#else
#include <openssl/rsa.h>
#include <openssl/bn.h>
#endif

static SSL_CTX* create_self_signed_ctx() {
    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) return nullptr;
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    EVP_PKEY* pkey = nullptr;
    EVP_PKEY_CTX* kctx = EVP_PKEY_CTX_new_from_name(nullptr, "RSA", nullptr);
    if (!kctx) { SSL_CTX_free(ctx); return nullptr; }
    if (EVP_PKEY_keygen_init(kctx) <= 0) { EVP_PKEY_CTX_free(kctx); SSL_CTX_free(ctx); return nullptr; }
    EVP_PKEY_CTX_set_rsa_keygen_bits(kctx, 2048);
    if (EVP_PKEY_keygen(kctx, &pkey) <= 0 || !pkey) { EVP_PKEY_CTX_free(kctx); SSL_CTX_free(ctx); return nullptr; }
    EVP_PKEY_CTX_free(kctx);
#else
    EVP_PKEY* pkey = EVP_PKEY_new();
    if (!pkey) { SSL_CTX_free(ctx); return nullptr; }
    RSA* rsa = RSA_new();
    BIGNUM* bn = BN_new();
    BN_set_word(bn, RSA_F4);
    RSA_generate_key_ex(rsa, 2048, bn, nullptr);
    BN_free(bn);
    EVP_PKEY_assign(pkey, EVP_PKEY_RSA, rsa);
#endif
    X509* cert = X509_new();
    if (!cert) { EVP_PKEY_free(pkey); SSL_CTX_free(ctx); return nullptr; }
    X509_set_version(cert, 2);
    ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);
    X509_gmtime_adj(X509_get_notBefore(cert), 0);
    X509_gmtime_adj(X509_get_notAfter(cert), 31536000L); // 1 year
    X509_set_pubkey(cert, pkey);
    X509_NAME* name = X509_get_subject_name(cert);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
        reinterpret_cast<const unsigned char*>("localhost"), -1, -1, 0);
    X509_set_issuer_name(cert, name);
    X509_sign(cert, pkey, EVP_sha256());
    SSL_CTX_use_certificate(ctx, cert);
    SSL_CTX_use_PrivateKey(ctx, pkey);
    X509_free(cert);
    EVP_PKEY_free(pkey);
    return ctx;
}

HttpDaemon::HttpDaemon(VolumeManager* volumeManager, 
                       const std::string& bindAddress, 
                       int httpPort, 
                       int httpsPort)
    : m_volumeManager(volumeManager)
    , m_bindAddress(bindAddress)
    , m_httpPort(httpPort)
    , m_httpsPort(httpsPort)
    , m_running(false)
    , m_volumeIndex(std::make_unique<VolumeIndex>(volumeManager))
    , m_apiVolumeIndex(std::make_unique<ApiVolumeIndex>(volumeManager))
    , m_dirIndex(std::make_unique<DirIndex>(volumeManager))
    , m_apiDirIndex(std::make_unique<ApiDirIndex>(volumeManager))
    , m_fileController(std::make_unique<FileController>(volumeManager))
    , m_apiFileController(std::make_unique<ApiFileController>(volumeManager)) {
    if (!volumeManager) throw std::invalid_argument("HttpDaemon::HttpDaemon: null volumeManager");
    Volume* assets = AssetsRegistry::instance().get();
    m_assetController = std::make_unique<AssetController>(assets);
}

HttpDaemon::~HttpDaemon() {
    stop();
}

bool HttpDaemon::initHttps() {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    const char* cert_file = "config/server-cert.pem";
    const char* key_file = "config/server-key.pem";
    m_ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (!m_ssl_ctx) return false;
    if (SSL_CTX_use_certificate_chain_file(m_ssl_ctx, cert_file) == 1 &&
        SSL_CTX_use_PrivateKey_file(m_ssl_ctx, key_file, SSL_FILETYPE_PEM) == 1) {
        if (SSL_CTX_check_private_key(m_ssl_ctx) == 1)
            return true;
    }
    SSL_CTX_free(m_ssl_ctx);
    m_ssl_ctx = create_self_signed_ctx();
    if (m_ssl_ctx) {
        std::cerr << "HTTPS: using built-in self-signed certificate (use -k or --insecure in curl)." << std::endl;
        return true;
    }
    return false;
}

void HttpDaemon::shutdownHttps() {
    if (m_ssl_ctx) {
        SSL_CTX_free(m_ssl_ctx);
        m_ssl_ctx = nullptr;
    }
}

bool HttpDaemon::start(bool withHttps) {
    if (m_running) {
        return false;
    }

    if (withHttps) {
        if (!initHttps()) {
            std::cerr << "HTTPS setup failed; HTTPS will not be available." << std::endl;
        }
    }

    m_running = true;

    // Start HTTP server thread
    m_serverThreads.emplace_back(&HttpDaemon::serverLoop, this, m_httpPort, false);

    // Start HTTPS server thread (only if we have SSL context)
    if (withHttps && m_ssl_ctx) {
        m_serverThreads.emplace_back(&HttpDaemon::serverLoop, this, m_httpsPort, true);
    }

    return true;
}

void HttpDaemon::stop() {
    if (m_running) {
        m_running = false;
        for (auto& thread : m_serverThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        m_serverThreads.clear();
        shutdownHttps();
    }
}

void HttpDaemon::serverLoop(int port, bool useHttps) {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        m_running = false;
        return;
    }
    
    // Set socket options
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, m_bindAddress.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid bind address: " << m_bindAddress << std::endl;
        close(serverSocket);
        m_running = false;
        return;
    }
    
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Failed to bind to " << m_bindAddress << ":" << port << std::endl;
        close(serverSocket);
        m_running = false;
        return;
    }
    
    if (listen(serverSocket, 10) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(serverSocket);
        m_running = false;
        return;
    }
    
    std::cout << (useHttps ? "HTTPS" : "HTTP") << " daemon listening on " 
              << m_bindAddress << ":" << port << std::endl;
    
    while (m_running) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            if (m_running) {
                std::cerr << "Failed to accept client connection" << std::endl;
            }
            continue;
        }
        
        // Handle request in a separate thread for better concurrency
        std::thread clientThread(&HttpDaemon::handleRequest, this, clientSocket, useHttps);
        clientThread.detach();
    }
    
    close(serverSocket);
}

void HttpDaemon::handleRequest(int clientSocket, bool useHttps) {
    char buffer[4096];
    ssize_t bytesRead = 0;
    SSL* ssl = nullptr;
    
    if (useHttps && m_ssl_ctx) {
        ssl = SSL_new(m_ssl_ctx);
        if (!ssl) {
            close(clientSocket);
            return;
        }
        SSL_set_fd(ssl, clientSocket);
        if (SSL_accept(ssl) <= 0) {
            SSL_free(ssl);
            close(clientSocket);
            return;
        }
        bytesRead = SSL_read(ssl, buffer, sizeof(buffer) - 1);
    } else {
        bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    }
    
    if (bytesRead <= 0) {
        if (ssl) { SSL_shutdown(ssl); SSL_free(ssl); }
        close(clientSocket);
        return;
    }
    
    buffer[bytesRead] = '\0';
    std::string request(buffer);
    
    // Parse HTTP request
    std::istringstream requestStream(request);
    std::string method, path, version;
    requestStream >> method >> path >> version;
    
    // Extract body if present
    std::string body;
    size_t bodyStart = request.find("\r\n\r\n");
    if (bodyStart != std::string::npos) {
        body = request.substr(bodyStart + 4);
    }
    
    // Generate response
    std::string response = generateResponse(method, path, body);
    
    if (ssl) {
        SSL_write(ssl, response.c_str(), static_cast<int>(response.length()));
        SSL_shutdown(ssl);
        SSL_free(ssl);
    } else {
        send(clientSocket, response.c_str(), response.length(), 0);
    }
    close(clientSocket);
}

std::string HttpDaemon::generateResponse(const std::string& method, const std::string& path, const std::string& body) {
    std::ostringstream response;
    
    // CORS headers for web interface
    std::string corsHeaders = 
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
    
    if (method == "OPTIONS") {
        // Handle preflight CORS request
        response << "HTTP/1.1 200 OK\r\n"
                 << corsHeaders
                 << "Content-Length: 0\r\n"
                 << "\r\n";
        return response.str();
    }
    
    try {
        const std::string decodedPath = percentDecodeHttpPath(path);
        std::string routeResponse = routeRequest(method, decodedPath, body);
        
        // Check if routeRequest already returned a complete HTTP response
        if (!routeResponse.empty() && routeResponse.find("HTTP/") == 0) {
            // Already a complete HTTP response (e.g., from FileController)
            // Just add CORS headers if not already present
            if (routeResponse.find("Access-Control-Allow-Origin") == std::string::npos) {
                // Insert CORS headers after the first line
                size_t firstLineEnd = routeResponse.find("\r\n");
                if (firstLineEnd != std::string::npos) {
                    routeResponse.insert(firstLineEnd + 2, corsHeaders);
                }
            }
            return routeResponse;
        }
        
        if (routeResponse.empty()) {
            response << "HTTP/1.1 404 Not Found\r\n"
                     << corsHeaders
                     << "Content-Type: application/json\r\n"
                     << "Content-Length: 23\r\n"
                     << "\r\n"
                     << "{\"error\":\"Path not found\"}";
        } else {
            // Determine content type based on path
            std::string contentType = "application/json";
            if (decodedPath.find("/volume/") == 0) {
                contentType = "text/html";
            }
            
            response << "HTTP/1.1 200 OK\r\n"
                     << corsHeaders
                     << "Content-Type: " << contentType << "\r\n"
                     << "Content-Length: " << routeResponse.length() << "\r\n"
                     << "\r\n"
                     << routeResponse;
        }
    } catch (const std::exception& e) {
        std::string errorMsg = "{\"error\":\"" + std::string(e.what()) + "\"}";
        response << "HTTP/1.1 500 Internal Server Error\r\n"
                 << corsHeaders
                 << "Content-Type: application/json\r\n"
                 << "Content-Length: " << errorMsg.length() << "\r\n"
                 << "\r\n"
                 << errorMsg;
    }
    
    return response.str();
}

std::string HttpDaemon::routeRequest(const std::string& method, const std::string& path, const std::string& body) {
    // Route to appropriate handler based on path
    
    // /asset/ -> AssetController
    if (path == "/asset" || path.find("/asset/") == 0) {
        if (method == "GET") {
            std::string pathInfo;
            if (path == "/asset") {
                pathInfo = ""; // Root directory
            } else if (path.find("/asset/") == 0) {
                pathInfo = path.substr(7); // After "/asset/"
            }
            return m_assetController->handleGet(pathInfo);
        }
        return "";
    }
    
    // /api/volume/ -> ApiVolumeIndex
    if (path == "/api/volume/" || path == "/api/volume") {
        if (method == "GET") {
            return m_apiVolumeIndex->handleGet();
        }
        return "";
    }
    
    // /api/volume/{index}/{path} -> ApiDirIndex or ApiFileController
    if (path.find("/api/volume/") == 0) {
        size_t volStart = 11; // After "/api/volume/"
        size_t volEnd = path.find('/', volStart);
        
        std::string indexStr;
        std::string pathInfo;
        
        if (volEnd == std::string::npos) {
            // No trailing slash - treat as root directory of volume
            // e.g., /api/volume/1 -> /api/volume/1/
            indexStr = path.substr(volStart);
            pathInfo = ""; // Root directory
        } else {
            indexStr = path.substr(volStart, volEnd - volStart);
            pathInfo = path.substr(volEnd + 1); // After "/api/volume/{index}/"
        }
        
        if (indexStr.empty()) {
            if (method == "GET")
                return m_apiVolumeIndex->handleGet();
            return "";
        }

        char* endPtr = nullptr;
        unsigned long parsed = std::strtoul(indexStr.c_str(), &endPtr, 10);
        if (endPtr == indexStr.c_str() || *endPtr != '\0') {
            return "{\"error\":\"Volume index must be a non-negative integer\"}";
        }
        const size_t volumeIndex = static_cast<size_t>(parsed);

        if (volumeIndex >= m_volumeManager->getVolumeCount()) {
            return "";
        }
        
        if (method == "GET") {
            // If pathInfo is empty, it's the root directory
            if (pathInfo.empty()) {
                return m_apiDirIndex->handleGet(volumeIndex, "");
            }
            
            // Try directory first, then file
            if (m_volumeManager->getVolume(volumeIndex)->isDirectory("/" + pathInfo)) {
                return m_apiDirIndex->handleGet(volumeIndex, pathInfo);
            } else {
                return m_apiFileController->handleGet(volumeIndex, pathInfo);
            }
        } else if (method == "POST") {
            std::vector<uint8_t> data(body.begin(), body.end());
            return m_apiFileController->handlePost(volumeIndex, pathInfo, data);
        } else if (method == "PUT") {
            std::vector<uint8_t> data(body.begin(), body.end());
            return m_apiFileController->handlePut(volumeIndex, pathInfo, data);
        } else if (method == "DELETE") {
            return m_apiFileController->handleDelete(volumeIndex, pathInfo);
        }
        return "";
    }
    
    // /volume/ -> VolumeIndex
    if (path == "/volume/" || path == "/volume") {
        if (method == "GET") {
            return m_volumeIndex->handleGet();
        }
        return "";
    }
    
    // /volume/{index}/{path} -> DirIndex or FileController
    if (path.find("/volume/") == 0) {
        size_t volStart = 8; // After "/volume/"
        size_t volEnd = path.find('/', volStart);
        
        std::string indexStr;
        std::string pathInfo;
        
        if (volEnd == std::string::npos) {
            // No trailing slash - treat as root directory of volume
            // e.g., /volume/1 -> /volume/1/
            indexStr = path.substr(volStart);
            pathInfo = ""; // Root directory
        } else {
            indexStr = path.substr(volStart, volEnd - volStart);
            pathInfo = path.substr(volEnd + 1); // After "/volume/{index}/"
        }
        
        if (indexStr.empty()) {
            if (method == "GET")
                return m_volumeIndex->handleGet();
            return "";
        }

        char* endPtr = nullptr;
        unsigned long parsed = std::strtoul(indexStr.c_str(), &endPtr, 10);
        if (endPtr == indexStr.c_str() || *endPtr != '\0') {
            return "<!DOCTYPE html><html><head><meta charset=\"utf-8\"/><title>Bad volume index</title></head><body><p>Volume "
                   "index must be a non-negative integer.</p></body></html>";
        }
        const size_t volumeIndex = static_cast<size_t>(parsed);

        if (volumeIndex >= m_volumeManager->getVolumeCount()) {
            return "";
        }
        
        if (method == "GET") {
            // If pathInfo is empty, it's the root directory
            if (pathInfo.empty()) {
                return m_dirIndex->handleGet(volumeIndex, "");
            }
            
            // Try directory first, then file
            if (m_volumeManager->getVolume(volumeIndex)->isDirectory("/" + pathInfo)) {
                return m_dirIndex->handleGet(volumeIndex, pathInfo);
            } else {
                return m_fileController->handleGet(volumeIndex, pathInfo);
            }
        }
        return "";
    }
    
    // Root status
    if (path == "/" || path == "/api/status") {
        std::ostringstream json;
        json << "{\"status\":\"ok\",\"volumes\":" << m_volumeManager->getVolumeCount() << "}";
        return json.str();
    }
    
    return "";
}
