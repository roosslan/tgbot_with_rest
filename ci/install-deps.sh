#!/usr/bin/env bash
# Установка зависимостей сборки на Ubuntu 24.04 (CI и локальная проверка в контейнере).
# Qt ставится через aqtinstall в $QT_ROOT, если его там ещё нет.
# Переменные: QT_VERSION (по умолчанию 6.10.1), QT_ROOT (по умолчанию $HOME/Qt)
set -euo pipefail

QT_VERSION="${QT_VERSION:-6.10.1}"
QT_ROOT="${QT_ROOT:-$HOME/Qt}"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

SUDO=""
if [ "$(id -u)" -ne 0 ]; then
    SUDO="sudo"
fi

export DEBIAN_FRONTEND=noninteractive
$SUDO apt-get update
$SUDO apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build git ca-certificates python3-venv \
    libboost-dev libboost-context-dev libboost-coroutine-dev libboost-system-dev \
    libcurl4-openssl-dev libssl-dev zlib1g-dev unixodbc-dev libicu-dev \
    libdbus-1-3 libgl1 libxkbcommon0 libfontconfig1

# Библиотека manapihttp нужна для линковки listchecker
$SUDO apt-get install -y "$REPO_ROOT/lib/manapihttp/manapihttp-v1.0.1ubuntu24.04-dbg.deb"

if [ ! -d "$QT_ROOT/$QT_VERSION/gcc_64" ]; then
    python3 -m venv /tmp/aqt-venv
    /tmp/aqt-venv/bin/pip install --quiet aqtinstall==3.3.0
    /tmp/aqt-venv/bin/aqt install-qt linux desktop "$QT_VERSION" linux_gcc_64 \
        -m qthttpserver qtwebsockets -O "$QT_ROOT"
fi

echo "Qt: $QT_ROOT/$QT_VERSION/gcc_64"
