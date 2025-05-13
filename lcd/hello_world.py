from RPLCD.i2c import CharLCD
import time

lcd = CharLCD('PCF8574', 0x27, cols=16, rows=2, backlight_enabled=True)

lcd.clear()
lcd.write_string('Hello, world!')
time.sleep(50)
lcd.clear()
