@echo off
rem Compila o terminal SIGPR no Windows com o GCC do MinGW
rem (o mesmo usado pelo Dev-C++ e pelo Code::Blocks). O gcc precisa estar no PATH.

gcc -std=c99 -Wall -Wextra -pedantic -Iinclude -O2 -o sigpr.exe src\main.c src\entrada.c src\arquivos.c src\producao.c src\qualidade.c src\rastreio.c src\relatorios.c
if errorlevel 1 goto erro

gcc -std=c99 -Wall -Wextra -pedantic -Iinclude -o teste_sigpr.exe tests\teste_sigpr.c src\entrada.c src\arquivos.c src\producao.c src\qualidade.c src\rastreio.c src\relatorios.c
if errorlevel 1 goto erro

teste_sigpr.exe
echo.
echo Para usar: sigpr.exe (execute dentro da pasta c, onde ficam dados e exportar)
goto fim

:erro
echo Falha na compilacao.

:fim
