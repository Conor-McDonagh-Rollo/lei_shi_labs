@echo off
ECHO Starting Server and Clients...

START output\Server.exe

FOR /L %%i IN (1,1,3) DO (
   START output\Client.exe
)