import os
import time

try:
    import RPi.GPIO as GPIO
except RuntimeError:
    print("Error importing RPi.GPIO!  This is probably because you need superuser privileges.  You can achieve this by using 'sudo' to run your script")

GPIO.cleanup()
GPIO.setmode(GPIO.BOARD)
#setup pins for use with gameboy switch and LED
GPIO.setup(7, GPIO.OUT)#gameboy power LED
GPIO.setup(8, GPIO.IN, pull_up_down=GPIO.PUD_UP)#detect if switch is off

GPIO.output(7, GPIO.HIGH)#turn power LED on
if(GPIO.input(8) == GPIO.LOW):
	os.system("poweroff")#shuts down the pi
GPIO.wait_for_edge(8, GPIO.FALLING)#detect if power is switched
os.system("poweroff")
