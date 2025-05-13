import time
import socket
import psutil
from RPLCD.i2c import CharLCD

lcd = CharLCD(i2c_expander='PCF8574', address=0x27, port=1, cols=16, rows=2)

def get_ip():
	try:
		s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		s.connect(("1.1.1.1", 80))
		ip = s.getsockname()[0]
		s.close()
	except:
		ip = "No Network"
	return ip

def get_cpu_temp():
	try:
		with open("/sys/class/thermal/thermal_zone0/temp", "r") as f:
			temp_str = f.readline()
			temp_c = float(temp_str) / 1000.0
			return f"{temp_c:.1f} C"
	except:
		return "N/A"

def get_cpu_usage():
	return f"{psutil.cpu_percent()}% used"

info_funcs = [
	lambda: ("IP Address:", get_ip()),
	lambda: ("CPU Temp:", get_cpu_temp()),
	lambda: ("CPU Usage:", get_cpu_usage())
]

try:
	while True:
		for get_info in info_funcs:
			line1, line2 = get_info()
			lcd.clear()
			lcd.write_string(line1[:16])
			lcd.crlf()
			lcd.write_string(line2[:16])
			time.sleep(3)
except KeyboardInterrupt:
	#lcd.clear()
	print("Exited.")

