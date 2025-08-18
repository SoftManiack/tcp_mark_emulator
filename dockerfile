# Используем официальный образ C++ с поддержкой CMake
FROM ubuntu:22.04 AS builder

# Устанавливаем необходимые зависимости
RUN apt-get update && \
    apt-get install -y \
    cmake \
    g++ \
    git \
    make \
    libspdlog-dev \
    && rm -rf /var/lib/apt/lists/*

# Копируем исходный код
WORKDIR /app

# Собираем проект
RUN mkdir build && \
    cd build && \
    cmake .. && \
    make
    
# Финальный образ
FROM ubuntu:22.04

# Устанавливаем только необходимые runtime зависимости
RUN apt-get update && \
    apt-get install -y \
    libspdlog-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/server .
COPY --from=builder /app/logs ./logs

# Открываем порт, который слушает сервер
EXPOSE 8080

# Запускаем сервер
CMD ["./server"]