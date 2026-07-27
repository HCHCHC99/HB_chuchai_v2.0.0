@echo off
chcp 65001 >nul
echo ========================================
echo   Modbus RTU 指令生成器 - 打包工具
echo ========================================
echo.

REM 检查 Python 是否安装
python --version >nul 2>&1
if errorlevel 1 (
    echo [错误] 未找到 Python，请先安装 Python
    pause
    exit /b 1
)

REM 检查 PyInstaller 是否安装
pip show pyinstaller >nul 2>&1
if errorlevel 1 (
    echo [提示] 正在安装 PyInstaller...
    pip install pyinstaller
    if errorlevel 1 (
        echo [错误] PyInstaller 安装失败
        pause
        exit /b 1
    )
)

echo [信息] 开始打包 modbus_tool.py ...
echo.

REM 执行打包命令
pyinstaller --onefile --name modbus_tool --distpath dist --workpath build --specpath . --console modbus_tool.py

if errorlevel 1 (
    echo.
    echo [错误] 打包失败！
    pause
    exit /b 1
)

echo.
echo ========================================
echo   [成功] 打包完成！
echo   输出位置: dist\modbus_tool.exe
echo ========================================
echo.

REM ===== 自动清理临时文件 =====
echo [信息] 正在清理临时文件...

REM 删除 build 文件夹
if exist build (
    rmdir /s /q build
    echo   - 已删除 build 文件夹
)

REM 删除 .spec 文件
if exist modbus_tool.spec (
    del /q modbus_tool.spec
    echo   - 已删除 modbus_tool.spec
)

REM 删除 __pycache__ 文件夹（如果存在）
if exist __pycache__ (
    rmdir /s /q __pycache__
    echo   - 已删除 __pycache__ 文件夹
)

echo [信息] 清理完成！
echo.
echo ========================================
echo   最终文件：dist\modbus_tool.exe
echo   文件大小：请查看实际大小
echo ========================================

pause