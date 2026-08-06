@echo off

REM dir /b /s %1/%2

cd %1

for %%g in (./%2) do (
   echo %%~fg
)
