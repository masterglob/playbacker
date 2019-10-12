@echo off
@set PUTTYPATH="C:\Program Files\PuTTY"
@set USER=tc
@set IP=192.168.7.80
@set PPK=C:\Users\MAO\Documents\_ssh\id-rsa-priv.ppk
@set TARGET_PATH=/home/%USER%/_dev

cd C:\Users\MAO\Documents\Arduino\Rasberry\PROJECTS\playbacker

@echo Running
%PUTTYPATH%\plink -i %PPK%  %USER%@%IP% "cd %TARGET_PATH%/out && sudo ./pbkr"
pause
