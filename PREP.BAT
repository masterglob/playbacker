@set PUTTYPATH="C:\Program Files\PuTTY"
@set USER=tc
@set IP=192.168.7.80
@set PPK=C:\Users\MAO\Documents\_ssh\id-rsa-priv.ppk

@echo Compiling
%PUTTYPATH%\plink -i %PPK%  %USER%@%IP% "tce-load -i gcc make compiletc wiringpi-dev libasound-dev"
@true