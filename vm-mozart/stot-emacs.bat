@echo off

set STOCKHOME=Y:\.root\opt\stockhausen-operette2

set PATH=%STOCKHOME%\bin;%PATH%

stow x-alice:/stoc/StotEmacsMain %1 %2 %3 %4 %5 %6 %7 %8 %9
