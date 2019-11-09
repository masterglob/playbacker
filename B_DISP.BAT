
cd C:\Users\MAO\Documents\Arduino\Rasberry\PROJECTS\playbacker
@CALL COPY


::@%PUTTYPATH%\pscp
%PUTTYPATH%\pscp -i %PPK% -r src/ %USER%@%IP%:/home/tc/_dev/src

@echo Compiling
%PUTTYPATH%\plink -i %PPK%  %USER%@%IP% "make bdisp -C %TARGET_PATH%"