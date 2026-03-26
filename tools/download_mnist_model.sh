#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODEL_PATH="$ROOT_DIR/assets/mnist-8.onnx"
URL="https://github.com/onnx/models/raw/main/validated/vision/classification/mnist/model/mnist-8.onnx"

mkdir -p "$ROOT_DIR/assets"

echo "Downloading MNIST ONNX model..."
curl -L "$URL" -o "$MODEL_PATH"
echo "Saved model to $MODEL_PATH"
