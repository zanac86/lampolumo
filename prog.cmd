set prog=D:\ST\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe

set fw=.\lampo\Release\lampo.elf

%prog% -c port=SWD -d %fw% -rst -run
