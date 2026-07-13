#!/bin/bash
set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
APP_NAME="WukongFreight"
VERSION="1.2.3"

QT_LIB_PATH="/opt/homebrew/opt/qt/lib"

echo "=========================================="
echo "  悟空快递运费计算系统 - macOS 打包脚本"
echo "=========================================="
echo ""

echo "[1/5] 清理旧的构建目录..."
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

echo "[2/5] 配置 CMake (Release)..."
cd "$BUILD_DIR"
cmake .. -DCMAKE_BUILD_TYPE=Release

echo "[3/5] 编译项目..."
cmake --build . --config Release -j$(sysctl -n hw.ncpu)

echo "[4/5] 部署 Qt 依赖并创建 DMG..."
macdeployqt "$APP_NAME.app" -dmg -libpath="$QT_LIB_PATH" -no-strip

DMG_NAME="${APP_NAME}_v${VERSION}_macOS_arm64.dmg"
mv "$APP_NAME.dmg" "$PROJECT_DIR/$DMG_NAME"

echo "[5/5] 创建 ZIP 便携版..."
cd "$BUILD_DIR"
ZIP_NAME="${APP_NAME}_v${VERSION}_macOS_arm64.zip"
zip -rq "$PROJECT_DIR/$ZIP_NAME" "$APP_NAME.app"

echo ""
echo "=========================================="
echo "  打包完成！"
echo "=========================================="
echo ""
echo "DMG 安装包: $PROJECT_DIR/$DMG_NAME"
echo "ZIP 便携版:  $PROJECT_DIR/$ZIP_NAME"
echo ""
echo "App Bundle: $BUILD_DIR/$APP_NAME.app"
echo ""

cd "$PROJECT_DIR"
ls -lh "$DMG_NAME" "$ZIP_NAME"
