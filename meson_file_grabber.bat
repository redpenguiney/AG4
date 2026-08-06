@echo off

for %%g in (%1/%2) do (
  echo %1/%%g
)

REM forfiles /s /m %1 /c "cmd /c echo @relpath"