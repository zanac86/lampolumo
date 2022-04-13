# Lampo Lumo (lamp light, esperanto)

Measure pulsation of lamp light

Original project https://mysku.ru/blog/diy/90736.html

## Connection

### OLED SSD1306 display on I2C

| STM | OLED |
|-----|------|
| 3.3 | VDD  |
| GND | GND  |
| B6  | SCK  |
| B7  | SDA  |

### TEMT6000 ambient light sensor to ADC1:0

| STM | TEMT6000 |
|-----|----------|
| 5V  | V        |
| GND | G        |
| A0  | S        |

## Firmware upload with `STM32CubeProgrammer`

```
cd /d D:\ST\STM32Cube\STM32CubeProgrammer\bin\
STM32_Programmer_CLI.exe -c port=SWD -d lampo.elf -rst -run
```

