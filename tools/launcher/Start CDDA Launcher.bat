@echo off
rem Opens the CDDA Fork Launcher (Launcher\cdda-launcher.ps1) without a console window.
start "" powershell.exe -NoProfile -ExecutionPolicy Bypass -STA -WindowStyle Hidden -File "%~dp0Launcher\cdda-launcher.ps1"
