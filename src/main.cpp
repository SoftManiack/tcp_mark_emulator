#include <sys/socket.h>  // socket(), bind(), listen(), accept()
#include <netinet/in.h>  // sockaddr_in, INADDR_ANY, htons()
#include <unistd.h>      // close(), read(), write()
#include <arpa/inet.h>   // inet_pton(), inet_ntop() (если нужно работать с IP-адресами)
#include <stdlib.h> 
#include <iostream>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <optional>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h> 

#define PORT 8080
#define BUFFER_SIZE 1024

class LaserMark {
    private:
        int serverSocket_ ;
        int clientSocket_;
        sockaddr_in address_;
        enum class Status {
            Mark,
            Error,
            Free
        };
        Status statusLaser_; 
        const std::string GET_STATUS_COMMAND_ =  "<GetStatus>\n";
        const std::string START_MARK_COMMAND_ =  "<StartMark>\n";

        std::string statusToString(Status status) {
            static const std::unordered_map<Status, std::string> statusMap = {
                {Status::Free, "Free"},
                {Status::Mark, "Mark"},
                {Status::Error, "Error"}
            };
            return statusMap.at(status);
        }

        std::optional<Status> stringToStatus(const std::string& str) {
            static const std::unordered_map<std::string, Status> strMap = {
                {"Free", Status::Free},
                {"Mark", Status::Mark},
                {"Error", Status::Error}
            };
            auto it = strMap.find(str);
            if (it != strMap.end()) {
                return it->second;
            }
            return std::nullopt;  
        }
    public:
        int startServer(){

            serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
            if (serverSocket_ == -1) {
                spdlog::error("Не удалось создать сокет");
                exit(EXIT_FAILURE);
            }
            
            address_.sin_family = AF_INET;
            address_.sin_addr.s_addr = INADDR_ANY;
            address_.sin_port = htons(PORT);

            if (bind(serverSocket_, (struct sockaddr*)&address_, sizeof(address_)) < 0) {
                std::cout << "Не удалось привязать сокет\n" <<  std::endl;
                spdlog::error("Не удалось привязать сокет");
                return 1;
            }

            if (listen(serverSocket_, 1) < 0) {
                std::cout << "Ошибка при попытке слушать сокет" <<  std::endl;
                spdlog::error("Ошибка при попытке слушать сокет");
                return 1;
            }

            std::cout << "Сервер запущен (макс. 1 подключение)" << std::endl;

            // Принимаем только одно подключение
            sockaddr_in clientAddr{};
            socklen_t clientAddrLen = sizeof(clientAddr);
            clientSocket_ = accept(serverSocket_, (sockaddr*)&clientAddr, &clientAddrLen);
            
            if (clientSocket_ == -1) {
                std::cerr << "Ошибка accept: " << strerror(errno) << std::endl;
                close(serverSocket_);
                return 1;
            }

            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
            std::cout << "Подключен клиент: " << clientIP << ":" << ntohs(clientAddr.sin_port) << std::endl;

            // Закрываем серверный сокет, чтобы больше никто не мог подключиться
            close(serverSocket_);  // <--- Важное изменение!

            char buffer[BUFFER_SIZE];
            while (true) {
                ssize_t bytesRead = recv(clientSocket_, buffer, BUFFER_SIZE - 1, 0);
                
                if (bytesRead <= 0) {
                    if (bytesRead == 0) {
                        std::cout << "Клиент отключился" << std::endl;
                    } else {
                        std::cerr << "Ошибка чтения: " << strerror(errno) << std::endl;
                    }
                    break;
                }

                // <GetStatus>
                // <SetTemplate;(21)2000000000343422(91)9020323>
                // <Mark>
                // <GetLog>
                
                buffer[bytesRead] = '\0';

                const char* response = "Сообщение получено\n";
                std::string str = buffer;   

                if(str == GET_STATUS_COMMAND_){
                    std::cout << "<GetStatus>" << buffer << std::endl;
                } else if (str == START_MARK_COMMAND_) {
                    std::cout << "<StartMark>" << buffer << std::endl;
                } else {
                    std::cout << "Получено: " << buffer << std::endl;
                }

                if (send(clientSocket_, response, strlen(response), 0) < 0) {
                    std::cerr << "Ошибка отправки: " << strerror(errno) << std::endl;
                    break;
                }
            }

            close(clientSocket_);
            return 0;

        }

        std::string handlerStatus(){
            return statusToString(statusLaser_);
        }
        void handlerSetTemplate(){
            
        }
        void handlerMark(){

        }   
        void handlerLog(){
            
        }
        
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

    LaserMark *device = new LaserMark;
    
    device->startServer();

    return 0;
}

/*
    mkdir build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make
    ./server

    nc localhost 12345
*/