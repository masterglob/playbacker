
cd C:\Users\MAO\Documents\Arduino\Rasberry\PROJECTS\playbacker
@CALL COPY

set MKTRGT=%1

::@%PUTTYPATH%\pscp
%PUTTYPATH%\pscp -i %PPK% -r src/ %USER%@%IP%:/home/tc/_dev/src

@echo Compiling
%PUTTYPATH%\plink -i %PPK%  %USER%@%IP% "make %MKTRGT% -C %TARGET_PATH%"
%PUTTYPATH%\plink -i %PPK%  %USER%@%IP% "sudo cp %TARGET_PATH%/out/pbkr /root/pbkr"