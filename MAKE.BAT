@cd C:\Users\MAO\Documents\Arduino\Rasberry\PROJECTS\playbacker
@cd _dev
@set /p BUILD=<build_id
@set /a BUILD=%BUILD%+1
@echo %BUILD%>build_id
@cd ..
echo BUILD=%BUILD%
pause

@CALL COPY

set MKTRGT=%1

@echo Compiling
%PUTTYPATH%\plink -i %PPK%  %USER%@%IP% "make %MKTRGT% -C %TARGET_PATH%" BUILD_ID=%Build%
:: %PUTTYPATH%\plink -i %PPK%  %USER%@%IP% "sudo cp %TARGET_PATH%/out/pbkr /root/pbkr"