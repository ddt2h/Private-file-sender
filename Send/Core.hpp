#include <SFML/Network.hpp>

#include <iostream>

#include <vector>

#include <thread>

#include <fstream>

#include <filesystem>

#include <Windows.h>

#include <shobjidl.h> 

#include "../src/Timer.hpp"

#ifndef SEND_CORE_HPP
#define SEND_CORE_HPP

#define TIMEOUT_SECONDS 7

std::vector<std::wstring> split(const std::wstring& str, int splitLength);

std::vector<std::wstring> getFilesPath();

class Timer {
public:
	Timer();

	bool timeStamp(double seconds);

	float secondsPassed();

	void restart();

private:
	sf::Clock clock;
};

struct UserDataNetwork {
	Engine::Timer timer;
	int id;
};

class Server {
public:
	Server();

	void run();

private:
	sf::TcpListener listener;

	int port = 22828; //default

	std::vector < sf::TcpSocket*> clients;
	std::vector<UserDataNetwork> clientsData;

	void onRunning();
};


class User {
public:
	User();

	void connect();

	void readFromFile();

	void sendFile(std::wstring imageBytes);

	std::wstring getFileExtension(std::wstring path);

	std::string readBytes(std::wstring path);

	void sendPing();

	void getFile();

	void checkForInput();

private:
	std::string filename = "cfg.txt";

	sf::IpAddress ip = "0";

	int port = 0;

	int rndKey = 0;

	sf::TcpSocket socket;

	void onRunning();

	void tryConnection();

	std::vector<std::wstring> chunksBuffer;
	std::wstring PREFIX = L"FILE_READY_TO_USE.SENDAPP23022024";

	//Ping stuff
	double currentPing;
	Engine::Timer pingTimer;
	Engine::Timer timeOutTimer;
	Engine::Timer sendPingTimer;

	void timeOutHandler();

	//Cypher stuff

	void encrypt(std::string &data, bool doDecrypt);

	
};


#endif // !SEND_CORE_HPP
