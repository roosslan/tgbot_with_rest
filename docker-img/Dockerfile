# Образ chanserv: сборка бота и listchecker из исходников + окружение для запуска.
# Контекст сборки - корень репозитория (см. compose.yaml), сабмодули должны быть получены:
#   git submodule update --init
# Сборка из директории docker-img:
#   docker compose build

ARG QT_VERSION=6.10.1

# ============================================================================ Этап сборки
FROM ubuntu:24.04 AS build
ARG QT_VERSION
ENV QT_VERSION=${QT_VERSION} \
    QT_ROOT=/opt/Qt

WORKDIR /src

# Зависимости и Qt (через aqtinstall) - отдельным слоем, он кэшируется, пока не меняется install-deps.sh
COPY ci/install-deps.sh ci/install-deps.sh
COPY lib/manapihttp/manapihttp-v1.0.1ubuntu24.04-dbg.deb lib/manapihttp/
RUN bash ci/install-deps.sh && rm -rf /var/lib/apt/lists/*

# Сборка chanserv и listchecker, при падении тестов образ не собирается
COPY . .
RUN BUILD_DIR=/build bash ci/build.sh

# ============================================================================ Этап запуска
FROM ubuntu:24.04 AS runtime
ARG QT_VERSION
ENV DEBIAN_FRONTEND=noninteractive

# Библиотеки для запуска. SQL-драйверы Qt (libqsqlpsql, libqsqlodbc) входят в сборку Qt,
# им нужны только libpq5 и unixodbc
COPY lib/manapihttp/manapihttp-v1.0.1ubuntu24.04-dbg.deb /tmp/
RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates curl supervisor python3 mc net-tools \
        libglib2.0-0t64 libzstd1 zlib1g libdbus-1-3 \
        libboost-context1.83.0 libboost-coroutine1.83.0 libboost-system1.83.0 \
        libcurl4t64 libssl3t64 libpq5 unixodbc odbcinst libicu74 \
        /tmp/manapihttp-v1.0.1ubuntu24.04-dbg.deb && \
    rm -rf /var/lib/apt/lists/* /tmp/*.deb

# Драйвер Microsoft ODBC 18, mssql-tools18 и PowerShell (для sendmail.ps1) ставятся из репозитория Microsoft:
# https://learn.microsoft.com/sql/connect/odbc/linux-mac/installing-the-microsoft-odbc-driver-for-sql-server
# ACCEPT_EULA=Y означает принятие лицензии Microsoft
RUN . /etc/os-release && \
    curl -fsSL -o /tmp/packages-microsoft-prod.deb https://packages.microsoft.com/config/ubuntu/${VERSION_ID}/packages-microsoft-prod.deb && \
    dpkg -i /tmp/packages-microsoft-prod.deb && rm /tmp/packages-microsoft-prod.deb && \
    apt-get update && \
    ACCEPT_EULA=Y apt-get install -y --no-install-recommends msodbcsql18=18.5.1.1-1 mssql-tools18=18.4.1.1-1 powershell && \
    ln -sf /usr/bin/pwsh /usr/bin/powershell && \
    rm -rf /var/lib/apt/lists/*

# Qt и собранные бинарники. /opt/chanserv не перекрывается volume'ами из compose
COPY --from=build /opt/Qt/${QT_VERSION}/gcc_64/lib /opt/qt/lib
COPY --from=build /opt/Qt/${QT_VERSION}/gcc_64/plugins /opt/qt/plugins
COPY --from=build /src/bin/chanserv /opt/chanserv/bin/chanserv
COPY --from=build /src/listchecker/bin/listchecker /opt/chanserv/listchecker/bin/listchecker

# Служебные скрипты: пути к ним хранятся в таблице settings (script_path_*)
RUN mkdir -p /var/log/supervisor /root/logs /home/rasa/chanserv/listchecker
COPY listchecker/listchecker_mem_used.sh /home/rasa/chanserv/listchecker/
COPY chanserv_mem_used.sh listchecker.sh reboot.sh /home/rasa/chanserv/
COPY docker-img/sendmail.ps1 /home/rasa/sendmail.ps1
COPY docker-img/supervisord.conf /etc/supervisor/conf.d/supervisord.conf
RUN chmod +x /home/rasa/chanserv/*.sh /home/rasa/chanserv/listchecker/*.sh

ENV LD_LIBRARY_PATH="/opt/qt/lib" \
    QT_PLUGIN_PATH="/opt/qt/plugins" \
    PATH="${PATH}:/opt/mssql-tools18/bin"
#ENV CHANSERV_CONFIG=/root/chanserv.toml
#ENV CHANSERV_LOGDIR=/root/logs

EXPOSE 53000 17900 8080

WORKDIR /root

# Одновременный запуск нескольких процессов в docker'е
CMD ["/usr/bin/supervisord"]
