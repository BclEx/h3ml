@echo off
xcopy /i/y "%1..\lib\cairo\%2\*.dll" "%3"
xcopy /i/y "%1..\lib\freeimage\%2\*.dll" "%3"