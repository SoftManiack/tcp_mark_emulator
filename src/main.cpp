#include <sys/socket.h>  // socket(), bind(), listen(), accept()
#include <netinet/in.h>  // sockaddr_in, INADDR_ANY, htons()
#include <unistd.h>      // close(), read(), write()
#include <arpa/inet.h>   // inet_pton(), inet_ntop() (если нужно работать с IP-адресами)
#include <stdlib.h> 
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h> 

#define PORT 8080
#define BUFFER_SIZE 1024

class LaserMarker {
    public:
        
    private:
};

class TCPServer {
    private:
        int serverSocket_ ;
        int clientSocket_;
        sockaddr_in address_;
        
    public:
        TCPServer(){

            serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
            if (serverSocket == -1) {
                perror("socket failed");
                exit(EXIT_FAILURE);
            }

             // Настройка адреса
            address_.sin_family = AF_INET;
            address_.sin_addr.s_addr = INADDR_ANY;
            address_.sin_port = htons(PORT);


        };
        handler(){
            
        }

    void start(){

    };
};

int main(){
    const std::string logFile = LOG_PATH "server.log";

    auto logger = spdlog::rotating_logger_mt(
        "server_logger", 
        logFile, 
        1024 * 1024,  // Макс. размер файла (1 МБ)
        5             // Кол-во файлов ротации
    );
    
    spdlog::set_default_logger(logger);
    spdlog::info("Тест ротируемых логов");

    return 0;
}

/*
    mkdir build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make
    ./server
*/