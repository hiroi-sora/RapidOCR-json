# RapidOCR-json 构建指南

本文档帮助如何在 Windows x64 上编译 RapidOCR-json 。

本文参考了 RapidAI官方的[编译说明](https://github.com/RapidAI/RapidOcrOnnx/blob/main/BUILD.md) 。

## 1. 前期准备

资源链接后面的(括弧里是版本)，请看清楚。

### 1.1 需要安装的工具：

- [Visual Studio 2019](https://learn.microsoft.com/zh-cn/visualstudio/releases/2019/release-notes) (Community)
- [Cmake](https://cmake.org/download/) (Windows x64 Installer)

## 2. 构建项目

1. 解压 `lib.7z` ，将其中两个文件夹 `onnxruntime-static` 和 `opencv-static` 解压到 `cpp/` 中。（注意，是直接放在 `cpp/opencv-static` ，而不是 `cpp/lib/opencv-static` ！）
2. 动动手指，点击运行 `generate-vs-project.bat` ，静等文件生成。
3. 打开 `build-win-vs2019-x64` ，用vs2019打开 `RapidOcrOnnx` 。
4. vs2019上方控制栏，将 `Debug` 改为 `Release` 。
5. 解决方案资源管理器 → ALL_BUILD → 常规：
   - 输出目录 → 改为 `$(ProjectDir)/Release` 。
   - 目标文件名 → 改为 `RapidOCR-json` 或你喜欢的名字。
6. 解决方案资源管理器 → ALL_BUILD → 调试：
   - 工作目录 → 改为 `$(ProjectDir)/Release` 。
7. 解决方案资源管理器 → ALL_BUILD → 高级：
   - 字符集 → 改为 `使用Unicode字符集` 。应用，关闭此页面。
9.  解决方案资源管理器 → RapidOcrOnnx → 常规：
    - 目标文件名 → 改为 `RapidOCR-json` 或你喜欢的名字。
10. 解决方案资源管理器 → RapidOcrOnnx → 高级：
    - 字符集 → 改为 `使用Unicode字符集` 。应用，关闭此页面。
11. F5尝试编译。如果有 `成功*个，失败0个……` 那就成功了。如果一个黑窗口一闪而过，那就正常。
12. 