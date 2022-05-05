import utime
import uselect
import picoexplorer as display
from pimoroni_i2c import PimoroniI2C
from breakout_encoder import BreakoutEncoder
from sys import stdin

# encoders variables
countr = 0
countl = 0
last_countr = 0
last_countl = 0;

frequency = 0
mode = ""
snr = 0
last_snr = 0

# serial reading stuff
buffer_line = ""
buffered_input = []
TERMINATOR = "\n"


#PINS_BREAKOUT_GARDEN = {"sda": 4, "scl": 5}
PINS_PICO_EXPLORER = {"sda": 20, "scl": 21}
#init i2c
i2c = PimoroniI2C(**PINS_PICO_EXPLORER)
#init encoders 
encr = BreakoutEncoder(i2c,address=0x0f)
encl = BreakoutEncoder(i2c,address=0x1f)

encr.set_brightness(1.0)
encl.set_brightness(1.0)

encr.set_led(255,0,0)
encl.set_led(255,0,0)

encr.clear_interrupt_flag()
encl.clear_interrupt_flag()


#display init and stuff
width = display.get_width()
height = display.get_height()
disp = bytearray(width * height * 2)
display.init(disp)


def read_knob(countr,countl):
    global last_countr
    global last_countl
    
    if(countr < last_countr):
        print("C4")
        last_countr = countr
    if(countr > last_countr):
        print("C1")
        last_countr = countr
    if(countl < last_countl):
        print("C3")
        last_countl = countl
    if(countl > last_countl):
        print("C2")
        last_countl = countl

def check_irqs():
    global countr
    global countl
    if encr.get_interrupt_flag():
        countr = encr.read()
        encr.clear_interrupt_flag()
        read_knob(countr,countl)
    if encl.get_interrupt_flag():
        countl = encl.read()
        encl.clear_interrupt_flag()
        read_knob(countr,countl)


#awesome routine from: https://github.com/GSGBen/pico-serial/
def read_serial_input():
    global buffered_input
    global buffer_line
    
    select_result = uselect.select([stdin], [], [], 0)
    while select_result[0]:
        input_character = stdin.read(1)
        buffered_input.append(input_character)
        select_result = uselect.select([stdin], [], [], 0)
    if TERMINATOR in buffered_input:
        line_ending_index = buffered_input.index(TERMINATOR)
        buffer_line = "".join(buffered_input[:line_ending_index])
        if line_ending_index < len(buffered_input):
            buffered_input = buffered_input[line_ending_index + 1 :]
        else:
            buffered_input = []
    else:
        buffer_line = ""

def check_command():
    global buffer_line
    global frequency
    global snr
    global mode
    
    if buffer_line != "":
        if buffer_line[0] == 'F':
            frequency = buffer_line[1:]
        elif buffer_line[0] == 'S':
            snr = int(buffer_line[1:])
        elif buffer_line[0] == 'M':
            mode = buffer_line[1:]



def display_clear():
    display.set_pen(0,0,0)
    display.clear()

def display_hud():
    display.set_pen(255,255,100)
    display.text("Frequency",94,10,200,3)
    display.set_pen(255,255,255)
    display.text(str(frequency),4,40,200,4)
    
    display.set_pen(255,255,100)
    display.text("SNR",192,70,200,3)
    display.set_pen(255,255,255)
    display.text(str(snr),4,90,200,4)
    
    display.set_pen(255,255,100)
    display.text("Mode",174,120,200,3)
    display.set_pen(255,255,255)
    display.text(str(mode),4,140,200,4)
    
    display.update()

def smooth_snr():
    global snr
    global last_snr 
    if(snr < last_snr):
        snr = last_snr - 1
    if snr < 0:
        snr = 0
    last_snr = snr
        


while True:
    display_clear()
    check_irqs()
    read_serial_input()
    check_command()
    smooth_snr()
    display_hud()
    utime.sleep(0.01)