#include "Core.hpp"

std::vector<std::wstring> split(const std::wstring& str, int splitLength) {
	int NumSubstrings = str.length() / splitLength;
	std::vector<std::wstring> ret;

	for (auto i = 0; i < NumSubstrings; i++)
	{
		ret.push_back(str.substr(i * splitLength, splitLength));
	}

	// If there are leftover characters, create a shorter item at the end.
	if (str.length() % splitLength != 0)
	{
		ret.push_back(str.substr(splitLength * NumSubstrings));
	}

	std::cout << "Splitted the file, parts - " << std::to_string(ret.size()) << std::endl;

	return ret;
}

std::vector<std::wstring> getFilesPath()
{
	std::vector<std::wstring> selectedFilePaths;

	// Initialize COM
	CoInitialize(NULL);

	IFileOpenDialog* pFileOpen;
	if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, (void**)&pFileOpen)))
	{
		// Set options for the file dialog
		DWORD dwOptions;
		pFileOpen->GetOptions(&dwOptions);
		pFileOpen->SetOptions(dwOptions | FOS_ALLOWMULTISELECT | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST);

		// Show the file open dialog
		if (SUCCEEDED(pFileOpen->Show(NULL)))
		{
			IShellItemArray* pItemArray;
			if (SUCCEEDED(pFileOpen->GetResults(&pItemArray)))
			{
				DWORD dwItemCount;
				pItemArray->GetCount(&dwItemCount);

				for (DWORD i = 0; i < dwItemCount; i++)
				{
					IShellItem* pItem;
					if (SUCCEEDED(pItemArray->GetItemAt(i, &pItem)))
					{
						PWSTR pszFilePath;
						if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
						{
							selectedFilePaths.push_back(pszFilePath);
							CoTaskMemFree(pszFilePath);
						}
						pItem->Release();
					}
				}
				pItemArray->Release();
			}
		}
		pFileOpen->Release();
	}

	CoUninitialize();

	return selectedFilePaths;
}

bool Timer::timeStamp(double seconds) {
	if (clock.getElapsedTime().asSeconds() >= seconds) {
		clock.restart();
		return true;
	}
	return false;
}

Timer::Timer() {
	clock.restart();
}

float Timer::secondsPassed() {
	return clock.getElapsedTime().asSeconds();
}

void Timer::restart() {
	clock.restart();
}


Server::Server() {

}

void Server::onRunning() {
	for (;;) {
		for (int client = 0; client < clients.size(); client++) {
			//Checking disconnections

			sf::Packet packet;

			std::wstring data;

			if (clients[client]->receive(packet) == sf::Socket::Done)
			{

				sf::Packet sendBackPacket = packet;

				packet >> data;

				if (data == L"ping") {
					//...Do something idk
					clientsData[client].timer.restart();
					clients[client]->send(packet);

					std::cout << "Client - " << client << " pinged" << std::endl;
				}
				else {
					clientsData[client].timer.restart();
					std::cout << "Got new file, resending" << std::endl;

					for (int i = 0; i < clients.size(); i++) {
						if (i != client)
							clients[i]->send(sendBackPacket);
					}

					//Tell user who send it that everything is ok
					//While user is sending something (big) respond with a ping, every timeStamp(1)

					if (clientsData[client].timer.timeStamp(1)) {
						sf::Packet pingPacket;
						std::wstring ping = L"ping";
						pingPacket << ping;

						clients[client]->send(pingPacket);
					}

					
				}

	
			}

			//If client's ping is too high (disconnected)

			if (clientsData[client].timer.secondsPassed() > TIMEOUT_SECONDS) {
				std::cout << "Client id: " << client << " timeout, deleting from queue" << std::endl;

				clients.erase(clients.begin() + client);
				clientsData.erase(clientsData.begin() + client);
			}
			
		}

		//_sleep(100);
	}
	
}

void Server::run() {
	
	//Leave thread for detecting disconnections

	std::thread discThread(&Server::onRunning, this);
	discThread.detach();


	for (;;) {
		if (listener.listen(port) != sf::Socket::Done)
		{
			std::cout << "Server is waiting for connections" << std::endl;
		}

		sf::TcpSocket *client = new sf::TcpSocket;
		if (listener.accept(*client) == sf::Socket::Done)
		{
			std::cout << "Got new connection from IP " + client->getRemoteAddress().toString() << std::endl;

			client->setBlocking(false);

			UserDataNetwork clientData;
			clientData.id = clients.size();

			clientsData.push_back(clientData);
			clients.push_back(client);

		}

	}
	
}

User::User() {
	readFromFile();

}

void User::encrypt(std::string &data, bool doDecrypt) {
	std::cout << "Encrypting, may take some time..." << std::endl;
	srand(rndKey);

	int min = -3;
	int max = 3;

	for (int i = 0; i < data.size(); i++) {
		int num = min + rand() % ((max + 1) - min);

		if (!doDecrypt)
			data[i] = data[i] + num;

		if (doDecrypt)
			data[i] = data[i] - num;
	}

	std::cout << "Encrypted data suc." << std::endl;
}

std::wstring User::getFileExtension(std::wstring path) {
	int charCount = 0;
	for (int i = path.size(); i > 0; i--) {

		if (path[i] == '\\') {
			return path.substr(1 + path.size() - charCount, charCount - 1);
		}

		charCount++;
	}

	return {};
}

void User::timeOutHandler() {
	for (;;) {
		if (sendPingTimer.timeStamp(1)) {
			sf::Packet pingPacket;
			pingPacket << L"ping";
			socket.send(pingPacket);

			pingTimer.restart();

			std::wstring title = L"Press F1 to load files, current ping: " + std::to_wstring(currentPing);
			SetConsoleTitle(&title[0]);

			if (timeOutTimer.secondsPassed() > TIMEOUT_SECONDS) {
				std::cout << "Cannot reach the server, trying to reconnect..." << std::endl;

				tryConnection();
				timeOutTimer.restart();
			}
		}
		
	}
}

void User::checkForInput() {
	//std::cout << "checking input" << std::endl;

	for (;;) {
		if (GetAsyncKeyState(0x70)) {

			std::vector<std::wstring> filePaths = getFilesPath();

			for (int i = 0; i < filePaths.size(); i++) {
				std::wstring extension = getFileExtension(filePaths[i]);

				std::string rawData = readBytes(filePaths[i]);

				encrypt(rawData, false);

				//rawData is encrypted
				sendFile(std::wstring(rawData.begin(), rawData.end()) + L"\\" + extension);
			}
	
		}

		Sleep(100);
	}
	
	
}

std::string User::readBytes(std::wstring path) {

	std::string out;
	
	std::ifstream ifs(path, std::ios::binary | std::ios::ate);
	std::ifstream::pos_type pos = ifs.tellg();

	out.resize(pos); 

	ifs.seekg(0, std::ios::beg);
	ifs.read(&out[0], pos);

	std::cout << "Read - " << std::to_string(out.size()) << " bytes from memory" << std::endl;

	return out;
}

void User::onRunning() {
	std::thread t1(&User::checkForInput, this);
	t1.detach();

	std::thread t2(&User::timeOutHandler, this);
	t2.detach();

	for (;;) {

		getFile();
	}
}

void User::tryConnection() {
	for (;;) {
		sf::Socket::Status status = socket.connect(ip, port);

		if (status == sf::Socket::Done)
		{
			std::cout << "Connected" << std::endl;

			break;
		}
		else {
			std::cout << "Cannot connect to server" << std::endl;
		}
		_sleep(30);

	}
} 

void User::connect() {

	tryConnection();

	onRunning();

}

void User::readFromFile() {
	std::ifstream infile(filename);

	std::string line;
	int lineNumber = 0;

	while (std::getline(infile, line))
	{
		if (lineNumber == 0)
			ip = line;

		if (lineNumber == 1)
			port = stoi(line);

		if (lineNumber == 2)
			rndKey = stoi(line);

		lineNumber++;
	}

	std::cout << "Read from config, IP - " + ip.toString() + ", port - " + std::to_string(port) + " encrypt seed - " + std::to_string(rndKey) << std::endl;
}

void User::sendFile(std::wstring image) {

	//Divide by chunks

	std::vector<std::wstring> chunks;
	size_t chunkSize = 1024 * 64; //16kb
	
	chunks = split(image, chunkSize);

	for (int i = 0; i < chunks.size(); i++) {
		sf::Packet packet;

		std::wstring prefix;

		if (i == chunks.size() - 1)
			prefix = PREFIX;

		packet << (prefix + chunks[i]);

		if (socket.send(packet) == sf::Socket::Done)
		{
			std::cout << "File send to server suc." << std::endl;
		}
	}

	
}

void User::getFile() {

	sf::Packet packet;

	std::wstring rawBytes;
	
	if (socket.receive(packet) == sf::Socket::Done) {
		packet >> rawBytes;

		//Ping check
		currentPing = pingTimer.secondsPassed();
		pingTimer.restart();
		timeOutTimer.restart();

		if (rawBytes == L"ping") {
			std::cout << "Got pinged from server" << std::endl;
			return;
		}

		std::wstring readPrefix = rawBytes.substr(0, PREFIX.size());

		if (readPrefix == PREFIX) {
			//Got last part of chunks

			//Unify

			std::cout << "Got end of chunk, size - " << std::to_string(rawBytes.size()) << std::endl;

			std::wstring extension = getFileExtension(rawBytes);

			rawBytes = rawBytes.substr(PREFIX.size(), rawBytes.size() - PREFIX.size());

			std::wstring unitedBytes;
			for (int i = 0; i < chunksBuffer.size(); i++) {
				unitedBytes += chunksBuffer[i];
			}
			//Not the best way I guess
			unitedBytes += rawBytes;

			//Creating folder
			std::filesystem::create_directories("Files");

			std::ofstream myfile;
			myfile.open(L"Files\\" + extension, std::ios_base::binary);

			std::string unitedBytesStr(unitedBytes.begin(), unitedBytes.end());

			//Decrypt
			encrypt(unitedBytesStr, true);

			myfile << unitedBytesStr.substr(0, unitedBytesStr.size() - extension.size() - 1);
			myfile.close();

			chunksBuffer.clear();
		}
		else {
			std::cout << "Got chunk - " + std::to_string(chunksBuffer.size()) << ", size - " << std::to_string(rawBytes.size()) << std::endl;

			chunksBuffer.push_back(rawBytes);

			return;
		}

						
	}
}