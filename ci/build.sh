#!/usr/bin/env bash
# Сборка chanserv, listchecker и запуск тестов.
# Переменные: QT_VERSION (по умолчанию 6.10.1), QT_ROOT (по умолчанию $HOME/Qt), BUILD_DIR (по умолчанию build)
set -euo pipefail

QT_VERSION="${QT_VERSION:-6.10.1}"
QT_ROOT="${QT_ROOT:-$HOME/Qt}"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$REPO_ROOT/build}"
QT_PREFIX="$QT_ROOT/$QT_VERSION/gcc_64"

export QT_QPA_PLATFORM=offscreen
export LD_LIBRARY_PATH="$QT_PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

echo "=== chanserv ==="
cmake -S "$REPO_ROOT" -B "$BUILD_DIR/chanserv" -G Ninja -DCMAKE_PREFIX_PATH="$QT_PREFIX"
cmake --build "$BUILD_DIR/chanserv"

echo "=== tests ==="
ctest --test-dir "$BUILD_DIR/chanserv" --output-on-failure

echo "=== listchecker ==="
cmake -S "$REPO_ROOT/listchecker" -B "$BUILD_DIR/listchecker" -G Ninja -DCMAKE_PREFIX_PATH="$QT_PREFIX"
cmake --build "$BUILD_DIR/listchecker"
