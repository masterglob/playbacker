cd "%~dp0"

@set IP=192.168.7.80
@set USER=tc
@set PPK=.ssh\id_rsa.ppk

@set PUTTYPATH="C:\Program Files\PuTTY"

@echo Compiling
%PUTTYPATH%\plink -i %PPK%  %USER%@%IP% "tce-load -i gcc gdb make compiletc wiringpi-dev libasound-dev samba4"
%PUTTYPATH%\plink -i %PPK%  %USER%@%IP% "sudo /usr/local/sbin/smbd -D || true"
