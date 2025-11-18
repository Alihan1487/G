@echo off
setlocal

REM --- Проверяем, установлен ли Python ---
python --version >nul 2>&1
IF %ERRORLEVEL% NEQ 0 (
    echo Python не найден. Скачиваем и устанавливаем...

    REM --- Скачиваем Python (Windows installer) ---
    set PYTHON_INSTALLER=python-installer.exe
    powershell -Command "Invoke-WebRequest -Uri https://www.python.org/ftp/python/3.12.2/python-3.12.2-amd64.exe -OutFile %PYTHON_INSTALLER%"

    REM --- Устанавливаем Python без запроса пользователя ---
    %PYTHON_INSTALLER% /quiet InstallAllUsers=1 PrependPath=1 Include_test=0

    REM --- Удаляем установщик ---
    del %PYTHON_INSTALLER%

    echo Python установлен.
) ELSE (
    echo Python найден.
)

REM --- Запуск локального сервера на порту 8000 ---
echo Запуск сервера и открытие index.html...
python -m http.server 8000 >nul 2>&1 & start http://localhost:8000/index.html

pause
