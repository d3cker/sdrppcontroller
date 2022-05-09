import utime
import uselect
import picoexplorer as display
import machine
from pimoroni_i2c import PimoroniI2C
from breakout_encoder import BreakoutEncoder
from sys import stdin

# encoders variables
countr = 0
countl = 0
last_countr = 0
last_countl = 0;

frequency = 0
snr = 0
last_snr = 0

modes = ["NFM","WFM","AM","DSB","USB","CW","LSB","RAW"]
mode = 0

#          Mode , Command button B, Command button Y, screen Xaxis offset
menus = [ ["Zoom In/Out", "C9","C6", 65],
          ["Page Left/Right", "C8","C7", 45] ]
menu_current = 0


# handle buttons 
buttons_states = [ 0,0,0,0 ]

# serial reading stuff
buffer_line = ""
buffered_input = []
TERMINATOR = "\n"

#not sure if it's needed
uart = machine.UART(0,9600)

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
    #global buffer_line
    global frequency
    global snr
    global mode
    
    if buffer_line != "":
        if buffer_line[0] == 'F':
            frequency = buffer_line[1:]
        elif buffer_line[0] == 'S' and len(buffer_line) > 1:
            snr = int(buffer_line[1:])
        elif buffer_line[0] == 'M' and len(buffer_line) > 1:
            if int(buffer_line[1:]) < 8 and int(buffer_line[1:]) > -1:
                mode = int(buffer_line[1:])
        elif buffer_line[0] == 'R':
            print("D SDR++ Controller")
            
def display_clear():
    display.set_pen(0,0,0)
    display.clear()

def display_hud():
    
    display.set_pen(255,255,100)
    display.text("Frequency",50,82,200,3)
    display.set_pen(255,255,255)
    display.text(str(frequency),4,108,200,4)
    
    display.set_pen(255,255,100)
    display.text("Mode",39,144,200,3)
    display.set_pen(255,255,255)
    display.text(modes[mode],135,140,200,4)    
    
    display.set_pen(255,255,100)
    display.text("SNR",58,206,200,3)  
    display.set_pen(255,255,255)
    display.text(str(snr),135,202,200,4)
    
    display.set_pen(100,180,255)
    display.text("<",1,175,200,2)
    display.text(">",234,175,300,2)
    display.rectangle(2,181,15,2)
    display.rectangle(width - 17,181,15,2)
    display.text(menus[menu_current][0],menus[menu_current][3],175,200,2)
   
    display.text("<",1,55,200,2)
    display.text(">",234,55,300,2)
    display.rectangle(2,61,15,2)
    display.rectangle(width - 17,61,15,2)
    display.text("Menu | Center | Mode", 24,55,200,2)

    display.set_pen(255,255,255)
    display.rectangle(2,8,width-4,30)  
    display.set_pen(0,0,0)
    display.rectangle(3,9,width-6,28)  
   
    snr_length = int(((width - 6) *  snr) / 90)
    
    display.set_pen(255,255,0)
    
    for x in range(3,snr_length + 2,2):
        if (x > (75 * (width - 6) / 90)):
            display.set_pen(255,0,0)
        elif (x > (12 * (width - 6) / 90)):
            display.set_pen(0,255,0)
        display.rectangle(x,9,1,28)
    
    display.update()

#not used 
def smooth_snr():
    global snr
    global last_snr 
    if(snr < last_snr):
        snr = last_snr - 1
    if snr < 0:
        snr = 0
    last_snr = snr

def read_buttons():
    global buttons_states
    global menu_current
    global mode
# button A - menu select
    if display.is_pressed(display.BUTTON_A) and not display.is_pressed(display.BUTTON_X):
        if(buttons_states[0] == 0):
            buttons_states[0] = 1
            menu_current += 1
            if menu_current == len(menus):
                menu_current = 0
    elif not display.is_pressed(display.BUTTON_A):
        buttons_states[0] = 0
# button X - mode cycle        
    if display.is_pressed(display.BUTTON_X) and not display.is_pressed(display.BUTTON_A):
        if(buttons_states[1] == 0):
            buttons_states[1] = 1
            print("C5")
    elif not display.is_pressed(display.BUTTON_X):
        buttons_states[1] = 0
# button B action
    if display.is_pressed(display.BUTTON_B):
        if(buttons_states[2] == 0):
            buttons_states[2] = 1
            print(menus[menu_current][1])
    elif not display.is_pressed(display.BUTTON_B):
        buttons_states[2] = 0
# button Y action
    if display.is_pressed(display.BUTTON_Y):
        if(buttons_states[3] == 0):
            buttons_states[3] = 1
            print(menus[menu_current][2])
    elif not display.is_pressed(display.BUTTON_Y):
        buttons_states[3] = 0
# button A + X - center waterfall
    if display.is_pressed(display.BUTTON_X) and display.is_pressed(display.BUTTON_A):
        if(buttons_states[0] == 0) and (buttons_states[1] == 0):
            buttons_states[0] = 1
            buttons_states[1] = 1
            print("CA")

def update_leds():
    brightness = (snr * 1.0) / 90
    encr.set_brightness(brightness)
    encl.set_brightness(brightness)
    if (snr > 12):
        encr.set_led(0,0,255)
        encl.set_led(0,0,255)
    else:
        encr.set_led(255,0,0)
        encl.set_led(255,0,0)


print("D SDR++ Controller")
while True:
    try:
        display_clear()
        check_irqs()
        read_buttons()
        read_serial_input()
        check_command()
        update_leds()
        #smooth_snr()
        #display.set_pen(255,255,255)
        #display.text(str(buffer_line),1,200,200,4)    
        display_hud()
    except:
        print("Something went wrong :/")
    
    utime.sleep(0.01)
    