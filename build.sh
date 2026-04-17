#!/bin/bash

# Zatrzymujemy skrypt, jeśli jakakolwiek komenda zwróci błąd
set -e

echo "🚀 Rozpoczynam budowanie projektu ticket_system..."

# Tworzymy folder build, jeśli nie istnieje
mkdir -p build
cd build

echo "📦 Krok 1: Pobieranie zależności (Conan)..."
conan install .. --output-folder=. --build=missing

echo "⚙️ Krok 2: Konfiguracja projektu (CMake)..."
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release

echo "🔨 Krok 3: Kompilacja kodu..."
cmake --build . --config Release

echo "🧪 Krok 4: Uruchamianie testów..."
ctest -C Release --output-on-failure

echo "✅ Wszystko gotowe! Pliki wykonywalne znajdują się w folderze build/"