
cd C:\Users\MAO\Documents\Arduino\Rasberry\PROJECTS\playbacker
call COPY.BAT

@echo Compiling
%PUTTYPATH%\plink -i %PPK%  %USER%@%IP% "make -C %TARGET_PATH% pbkr"
%PUTTYPATH%\plink -i %PPK%  %USER%@%IP% "sudo cp %TARGET_PATH%/out/pbkr /root/pbkr"
