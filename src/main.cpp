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
#include <regex>
#include <chrono>
#include <thread>
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
        const char* responseSetTemplate_ = "Шаблон Установлен\n";
        struct CodeData {
            std::unordered_map<std::string, std::string> fields;  
        };
        CodeData template_;

        const char* statusToChar(Status status) {
            static const std::unordered_map<Status, std::string> statusMap = {
                {Status::Free, "Free"},
                {Status::Mark, "Mark"},
                {Status::Error, "Error"}
            };
            return (statusMap.at(status)).c_str();
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
        bool parseTemplate(const std::string& input) {
            std::regex pattern(R"(^Template:\{([A-Za-z0-9]+:[0-9]{1,200}(?:,[A-Za-z0-9]+:[0-9]{1,200})*)\}$)");
            std::smatch matches;

            std::string cleanedInput;
            for (char c : input) {
                if (c != '\n') cleanedInput += c;
            }
            
            if (!std::regex_match(cleanedInput, matches, pattern)) {
                return false;
            }

            std::string content = matches[1].str();
            std::regex pairRegex(R"(([A-Za-z0-9]+):([0-9]{1,200}))");
            auto begin = std::sregex_iterator(content.begin(), content.end(), pairRegex);
            auto end = std::sregex_iterator();
            
            for (auto it = begin; it != end; ++it) {
                std::smatch fieldMatch = *it;
                template_.fields[fieldMatch[1].str()] = fieldMatch[2].str();  // {Имя: Значение}
            }

            return true;
        }
    public:
        LaserMark(){
            statusLaser_ = Status::Free;
        }
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
                // <StartMark>
                // <GetLog>
                
                //Template:{Code1:120012020101221,Code2:210323233223232332101221,Code3:1000}
                buffer[bytesRead] = '\0';
                
                const char* response = "Неверный формат\n";

                std::string str = buffer;   

                bool isTemplate = parseTemplate(str);
                
                if(str == GET_STATUS_COMMAND_){
                    handlerStatus();
                } else if(str == START_MARK_COMMAND_) {
                    handlerMark();
                } else if(isTemplate){
                    for (const auto& [fieldName, fieldValue] : template_.fields ) {
                        std::cout << fieldName << " = " << fieldValue << "\n";
                    }
                    if (send(clientSocket_, responseSetTemplate_, strlen(responseSetTemplate_), 0) < 0) {
                        std::cerr << "Ошибка отправки: " << strerror(errno) << std::endl;
                        break;
                    }
                } else {
                    if (send(clientSocket_, response, strlen(response), 0) < 0) {
                        std::cerr << "Ошибка отправки: " << strerror(errno) << std::endl;
                        break;
                    }
                }
            }

            close(clientSocket_);
            return 0;
        }

        void handlerStatus(){
            const char* response = statusToChar(Status::Free); 

            if (send(clientSocket_, response, strlen(response), 0) < 0) {
                spdlog::error("Ошибка при попытке получить статус");
            }

        }
        void handlerMark(){
            
            const char* responseStart = "Маркировка началась\n";
            const char* responseEnd = "Маркировка закончена\n";

            spdlog::info("Маркировка началась");

            if (send(clientSocket_, responseStart, strlen(responseStart), 0) < 0) {
                spdlog::error("Маркировка");
            }

            statusLaser_ = Status::Mark;
            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
            statusLaser_ = Status::Free;

            spdlog::info("Маркировка закончена");

            if (send(clientSocket_, responseEnd, strlen(responseEnd), 0) < 0) {
                spdlog::error("Маркировка");
            }

        }   
        void handlerGetLog(){
            
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
