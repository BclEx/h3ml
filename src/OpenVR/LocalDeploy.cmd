@echo off
xcopy /i/y "%1..\lib\openvr\win%2\*.dll" "%3"
xcopy /i/y "%1..\lib\*.png" "%3"