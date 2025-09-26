#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include "common/Types.hpp"

namespace fastbev {
	class UDPServer
	{
	private:
		int serverFd;
		struct sockaddr_in address{};
		int port;
		int connections;
	public:
		explicit UDPServer(int port = 8080);
		~UDPServer();

		bool initialize();

		void sendDetections( const std::vector<BoundingBox>& detections,
							const std::string& clientIp = "10.255.255.254", int clientPort = 8081);
		void closeSocket();
	};
};
