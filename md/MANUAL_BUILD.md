# 悟空快递运费计算系统 - 编译和打包指南

**日期**: 2026-07-09  
**状态**: 代码已就绪，需要手动编译

---

## ⚠️ 编译环境问题

当前系统未检测到 CMake，需要手动配置编译环境。

---

## 🔧 编译方法

### 方法 1: 使用 Qt Creator（推荐）

1. **打开 Qt Creator**
2. **文件 → 打开文件或项目**
3. **选择** `C:\Users\ccc\Desktop\wukong-main\CMakeLists.txt`
4. **等待 CMake 配置完成**
5. **点击"构建"按钮**（锤子图标）
6. **选择 Release 配置**
7. **等待编译完成**

编译成功后，可执行文件位置：
```
C:\Users\ccc\Desktop\wukong-main\build\Release\WukongFreight.exe
```

---

### 方法 2: 使用 Visual Studio

1. **打开 Visual Studio 2022**
2. **文件 → 打开 → 文件夹**
3. **选择** `C:\Users\ccc\Desktop\wukong-main`
4. **等待 CMake 配置**
5. **生成 → 全部生成**

---

### 方法 3: 手动安装 CMake

1. 下载 CMake: https://cmake.org/download/
2. 安装后添加到 PATH
3. 运行：
```batch
cd C:\Users\ccc\Desktop\wukong-main
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

---

## 📦 编译后打包

编译成功后，运行打包脚本：

```powershell
cd C:\Users\ccc\Desktop\wukong-main
.\simple_package.ps1
```

或手动打包：

```powershell
# 创建打包目录
mkdir package

# 复制主程序
copy build\Release\WukongFreight.exe package\

# 复制 Qt DLL（根据你的 Qt 安装路径）
copy C:\Qt\6.x.x\msvc*\*\bin\Qt6*.dll package\

# 复制平台插件
mkdir package\platforms
copy C:\Qt\6.x.x\msvc*\*\bin\..\..\plugins\platforms\qwindows.dll package\platforms\

# 创建压缩包
Compress-Archive -Path package\* -DestinationPath WukongFreight_v1.0.0.zip
```

---

## ✅ 快速检查清单

- [ ] 已安装 Qt 6.x
- [ ] 已安装 Visual Studio 2022 或 Qt Creator
- [ ] 已安装 CMake（可选，IDE 自带）
- [ ] 编译成功
- [ ] 打包成功

---

## 📞 需要帮助？

如果编译遇到问题，请：

1. 确认已安装 Visual Studio 2022（包含 C++ 桌面开发）
2. 确认已安装 Qt 6.x
3. 检查 CMakeLists.txt 配置
4. 查看详细错误信息

---

**代码状态**: ✅ 已就绪  
**编译状态**: ⏳ 待手动编译  
**打包状态**: ⏳ 待编译完成后打包
