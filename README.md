# FreqMul
PWM frequency multiplier for display backlight

             100n
      ┌───────┤├───────┐
      │ ┌────────────┐ │
      │ │ 1        8 ├─┴─ VCC
      │ │ 2        7 ├─ PB2 (INT0)  INPUT     
      │ │ 3        6 ├─ PB1 (OC1A)  OUTPUT
 GND ─┴─┤ 4        5 │
        └────────────┘
           ATTiny45
