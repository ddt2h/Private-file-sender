#include <iostream>
#include "Send/Core.hpp"


int main(){
    //Open console
    AllocConsole();
    freopen("conin$", "r", stdin);
    freopen("conout$", "w", stdout);
    freopen("conout$", "w", stderr);

    for (;;) {
        std::cout << "Run a server? - Y/N" << std::endl;

        char answer;
        std::cin >> answer;

        if (answer == 'Y' || answer == 'y') {
            std::cout << "Running server" << std::endl;
            //...
            Server server;

            server.run();

            break;
        }
        if (answer == 'N' || answer == 'n') {
            std::cout << "Running client" << std::endl;
            //...
            User user;
            user.connect();
            break;
        }
        std::cout << "Incorrect input (idiot)" << std::endl;
    }

   
}
