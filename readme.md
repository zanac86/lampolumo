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


## Timings

Ext 8MHz -> HCLK 72 Mhz

APB1 Timer clocks = 72 Mhz
APB2 Timer clocks = 72 Mhz

таймеры TIM2, TIM3, TIM4 работают от APB1 

Это не период между семплами, а время преобразования
зависит от тактовой частоты подаваемой на АЦП
ADC1 = 9 Mhz
а время преобразования задается в тиках этой частоты
ADC1 conversion time - 71.5 cycles - 7,94444 us



BUF - 3000 samples
Sample ADC read period - 100 us - 10 kHz
Fill time - 0.6 s

TIM2 - 1 s

Период между сбором семплов АЦП
TIM3 - 
делители 144-1, 50-1  - 10000 кГц - 100.0 мкс
делители 225-1, 100-1 -  3200 кГц - 312.5 мкс


