#include "udp.hpp"

fastbev::UDPServer::UDPServer(int port) : port(port) {
    serverFd = -1;
}

fastbev::UDPServer::~UDPServer() {
    closeSocket();
}

bool fastbev::UDPServer::initialize() {
    if ((serverFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cerr << "socket() failed" << std::endl;
        return false;
    }

    int broadcast = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        std::cerr << "setsockopt() failed" << std::endl;
    }

    std::cout << "UDP server listening on port " << port << std::endl;

    return true;
}

void fastbev::UDPServer::sendDetections(const std::vector<BoundingBox>& detections, const std::string &clientIp, int clientPort) {
    if (serverFd < 0) return;

    std::ostringstream ss;

    for (const auto& box : detections) {
        ss << box.position.x << " ";
        ss << box.position.y << " ";
        ss << box.position.z << " ";
        ss << box.size.w << " ";
        ss << box.size.l << " ";
        ss << box.size.h << " ";
        ss << box.z_rotation << " ";
        ss << box.id << " ";
        ss << box.score << " ";
        ss << "\n";
    }

    const std::string msg = ss.str();

    struct sockaddr_in clientAddr{};
    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(clientPort);
    inet_pton(AF_INET, clientIp.c_str(), &clientAddr.sin_addr);

    sendto(serverFd, msg.c_str(), msg.length(), 0,
            reinterpret_cast<struct sockaddr*>(&clientAddr), sizeof(clientAddr));

    std::cout << "UDP server sent " << msg.size() << std::endl;
}

void fastbev::UDPServer::closeSocket() {
    if (serverFd >= 0) {
        close(serverFd);
        serverFd = -1;
    }
}


