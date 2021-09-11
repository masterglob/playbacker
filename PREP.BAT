@set IP=192.168.1.6
@set USER=tc
@set PPK=C:\Users\MAO\Documents\_ssh\id-rsa-priv.ppk

@set PUTTYPATH="C:\Program Files\PuTTY"

@echo Compiling
%PUTTYPATH%\plink -i %PPK%  %USER%@%IP% "tce-load -i gcc make compiletc wiringpi-dev libasound-dev samba4"
%PUTTYPATH%\plink -i %PPK%  %USER%@%IP% "sudo /usr/local/sbin/smbd -D || true"
