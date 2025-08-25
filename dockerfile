FROM ubuntu:22.04 AS builder

# Устанавливаем зависимости для сборки
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    && rm -rf /var/lib/apt/lists/*

# Сначала копируем только CMake файлы для кэширования
WORKDIR /app
COPY CMakeLists.txt ./
COPY src/ ./src/

# Создаем директорию для сборки и собираем проект
RUN mkdir -p build && \
    cd build && \
    cmake .. && \
    make -j$(nproc)

# Финальный образ
FROM ubuntu:22.04

# Устанавливаем только необходимые runtime зависимости
RUN apt-get update && apt-get install -y \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

# Создаем пользователя для безопасности
RUN useradd -m appuser

# Создаем директорию для логов
RUN mkdir -p /app/logs && chown appuser:appuser /app/logs

# Копируем собранный бинарник из builder stage
COPY --from=builder --chown=appuser:appuser /app/build/server /app/server

# Переключаемся на непривилегированного пользователя
USER appuser

WORKDIR /app

# Устанавливаем переменную окружения для логов
ENV LOG_DIR=/app/logs

# Экспортируем порт (замените на нужный вам порт)
EXPOSE 8080

# Запускаем приложение
CMD ["./server"]