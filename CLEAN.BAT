cd "%~dp0"

@set IP=192.168.7.80
@set USER=tc
@set PPK=.ssh\id_rsa.ppk

@echo off
@set PUTTYPATH="C:\Program Files\PuTTY"
@set TARGET_PATH=/home/%USER%/_dev


::@%PUTTYPATH%\pscp
%PUTTYPATH%\pscp -i %PPK% -r src/ %USER%@%IP%:/home/tc/_dev/src

@echo Compiling
%PUTTYPATH%\plink -i %PPK%  %USER%@%IP% "make -C %TARGET_PATH% clean all"