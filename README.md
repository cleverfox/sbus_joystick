## stm8s103k3 based joystick with S.Bus interface

This is firmware to custom joystick, which I made for Beholder DS-1 gimbal.
I need S.Bus joystick because I use wireless remote controlles speaking
S.Bus sometimes, so I don't want to reconfigure gimbal each time.

UART output must be inverted for S.Bus, I use NPN transistor for it.

```
                       ----{4.7k}----> VCC 3.3 or 5V
                     /
                    *-----------> S.Bus output
                    |
                |  /
  TX--{ 20k }---|<
                | _\|
                    \
                     |
                   __|__
`````

To compile it use SDCC for stm8.
This fw supports 5 channels, but it can be extended easily.


