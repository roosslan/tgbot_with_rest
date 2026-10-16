Chanserv (former #### One 21.1) - Jira and Trello CPP API and a Telegram Bot (++)\
Uses PostgreSQL as main DB and MS SQL QODBC3 as a backup DB       \
Buttons are saved in pgsql as well
\
\
Aug 2024    v.1\
Sep 2024    v.2\
Oct 2024    v.3\
Nov 2024    v.4\
Dec 2024    v.5\
Jan 2025    v.6\
Feb 2025    v.7\
Mar 2025    v.8\
Apr 2025    v.9\
May 2025    v.10\
Jun 2025    v.11\
Jul 2025    v.12\
Aug 2025    v.13\
Sep 2025    v.14\
Oct 2025    v.15\
Nov 2025    v.16\
Dec 2025    v.17\
Jan 2026    v.18\
Feb 2026    v.19\
Mar 2026    v.20\
Apr 2026    v.21\
May 2026    v.21.1
<br/>
- C++ 20
- CMake 3.28 (built using Qt Creator 14.0.2)
- Qt6 (built on 6.10.1)
- requires installed QTerminal package (except in docker container)
- boost
- curl
- openssl
- tgbot-cpp, toml++ (сабмодули в lib/)

### Сборка

Перед сборкой необходимо получить сабмодули:

    git submodule update --init

Драйвер Microsoft ODBC 18 для SQL Server и mssql-tools18 в репозитории не хранятся. Их необходимо установить из репозитория Microsoft по инструкции:
https://learn.microsoft.com/sql/connect/odbc/linux-mac/installing-the-microsoft-odbc-driver-for-sql-server \
В Docker-образе они ставятся автоматически (docker-img/Dockerfile).

### Docker

Образ собирается из исходников: этап сборки ставит зависимости и Qt скриптом `ci/install-deps.sh`, собирает chanserv и listchecker и прогоняет тесты, этап запуска содержит только библиотеки, Qt (вместе с SQL-драйверами PostgreSQL и ODBC) и бинарники в `/opt/chanserv`. Контекст сборки - корень репозитория. Для сборки необходимо получить сабмодули и выполнить в директории `docker-img`:

    docker compose build

Конфиг (`/root/chanserv.toml`) и логи подключаются volume'ами из `compose.yaml`.

### Тесты и CI

Модульные тесты (QtTest) находятся в `tests/` и собираются вместе с ботом (опция `CHANSERV_BUILD_TESTS`, по умолчанию включена). Для запуска после сборки:

    ctest --test-dir build --output-on-failure

GitHub Actions (`.github/workflows/ci.yml`) на каждый PR и push в `trunk` собирает chanserv и listchecker и запускает тесты на Ubuntu 24.04 с Qt 6.10.1. Те же шаги можно выполнить локально в контейнере ubuntu:24.04 скриптами `ci/install-deps.sh` и `ci/build.sh`.

### Конфигурация

- `docker-img/chanserv.toml` — подключение к PostgreSQL, адрес Jira (`[jira] browse_url`), секрет вебхуков (`[jira] webhook_secret`), время жизни сессий (`session_ttl_hours`). Путь к файлу задаётся переменной окружения `CHANSERV_CONFIG`.
- Таблица `settings` PostgreSQL — токены Telegram, Trello, Jira и пути к скриптам.
- `src/sensitive_data.h.example` — шаблон для `src/sensitive_data.h`, сам файл в git не добавляется.
- Вебхук в Jira необходимо настроить на адрес `http://<host>:<port_jira_webhook>/UPD/${issue.key}?token=<webhook_secret>`. Запросы без верного token отклоняются с кодом 403. Если `webhook_secret` пустой, приём вебхуков отключён.
- `docker-img/sendmail.ps1` — отправка HTML-писем через корпоративный SMTP. Параметры SMTP задаются переменными окружения `CHANSERV_SMTP_*` (см. `docker-img/run.env`), пароль передаётся только через `CHANSERV_SMTP_PASSWORD`.

\
Minimal Qt configuration:

configure -sql-odbc -sql-psql -skip qtwebengine -skip qtquick3d -skip qtmultimedia -skip qtgraphs -skip qtquick3dphysics -skip qtquickeffectmaker -skip qtspeech -skip qtdoc -skip qtquick -skip qtdeclarative -skip qtquickcontrols -skip qtquickcontrols2 -skip qtlocation -skip qtlottie -skip qtmqtt -skip qtopcua -skip qtquicktimeline -skip qtvirtualkeyboard -skip qtwebview -skip qttools -skip qttranslations -skip qt3d -skip qtcharts -skip qtcoap -skip qtdatavis3d -skip qtgrpc -skip qtremoteobjects -skip qtscxml -skip qtsensors -skip qtwayland -skip qtwebchannel

\
![x86](https://github.com/roosslan/tgbot_with_rest/blob/trunk/chanserv.gif?raw=true)
