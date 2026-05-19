Project2 (DoodleJump)

说明
----
本项目为 Windows 下使用 Visual Studio 构建的 C++ 桌面游戏源码。

要点
----
- 已在项目配置中设置 `_WIN32_WINNT=0x0600` 和 `WINVER=0x0600`，代码中也包含了对应宏以启用 `GetTickCount64`。 
- 文件编码已统一为 UTF-8 带 BOM（便于在 Visual Studio 中正确显示中文注释）。
- 依赖：
  - Windows API（MessageBox、mciSendString、winmm 等）
  - `graphics.h`（可能来自 EasyX 或其他图形库），请确保在目标机器安装/配置该库和头文件
  - 链接库：`winmm.lib`（已在 `audio.cpp` 中通过 `#pragma comment(lib, "winmm.lib")` 指定）
- 资源：`player.png`、`platform.png` 等图片放在项目文件夹内，请保持相对路径不变。

构建 (建议在 Windows + Visual Studio 中执行)
----
1. 使用 Visual Studio 打开 `Project2.slnx`（若路径不同，请打开对应的解决方案文件）。
2. 选择合适的配置（Debug/Release）与平台（Win32/x64）。
3. 若缺少 Platform Toolset，按提示安装或选择对应版本（本工程记录为 v145）。
4. 生成/重新生成解决方案。

注意事项
----
- 请不要包含编译产物（如 `x64/Debug/`、`.exe`、`.ilk`、`.pdb` 等）到源码包中。若需要直接运行，可另外提供预编译的 `exe`（注意平台兼容性与杀毒提示）。
- 若目标是跨平台编译（非 Windows），需替换或移除 Windows 专有 API（`MessageBox`、`mciSendString`、`GetTickCount64` 等）。

打包说明
----
此仓库已为你生成压缩包 `Project2_src.zip`，位于仓库根目录，已排除常见的构建产物。

如需我：
- 帮你把 `Project2_src.zip` 上传到某个云盘或生成 GitHub Release，我可以继续操作；
- 或者进一步生成一个包含预编译二进制的运行包（需你提供目标平台信息），告诉我即可。