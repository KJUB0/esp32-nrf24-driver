import matplotlib
matplotlib.use('TkAgg') 
import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import threading
import time

# --- CONFIGURATION ---
# Check your port! usually /dev/ttyUSB0 or /dev/ttyUSB1
SERIAL_PORT = '/dev/ttyUSB0' 
BAUD_RATE = 115200
NUM_CHANNELS = 80 

# --- SETUP SERIAL ---
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
    print(f"Connected to {SERIAL_PORT} at {BAUD_RATE} baud.")
except Exception as e:
    print(f"Error connecting to serial port: {e}")
    exit()

current_data_left = [0] * NUM_CHANNELS
current_data_right = [0] * NUM_CHANNELS

# --- PLOT SETUP ---
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))
fig.suptitle('NRF24 Spectrum Analyzer')

bar_left = ax1.bar(range(NUM_CHANNELS), current_data_left, color='cyan')
ax1.set_title('Left Radio (DATA_LEFT)')
ax1.set_ylim(0, 15)
ax1.set_ylabel('Signal Strength')

bar_right = ax2.bar(range(NUM_CHANNELS), current_data_right, color='magenta')
ax2.set_title('Right Radio (DATA_RIGHT)')
ax2.set_ylim(0, 15)
ax2.set_ylabel('Signal Strength')
ax2.set_xlabel('Channel Index')

# --- DATA READING THREAD ---
def read_serial():
    global current_data_left, current_data_right
    while True:
        try:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line.startswith("DATA_LEFT:"):
                    raw_values = line.split(':')[1].split(',')
                    current_data_left = [int(x) for x in raw_values if x.isdigit()]
                elif line.startswith("DATA_RIGHT:"):
                    raw_values = line.split(':')[1].split(',')
                    current_data_right = [int(x) for x in raw_values if x.isdigit()]
        except Exception as e:
            print(f"Serial Read Error: {e}")
            time.sleep(0.01)

thread = threading.Thread(target=read_serial, daemon=True)
thread.start()

def update_plot(frame):
    data_l = current_data_left[:NUM_CHANNELS] + [0]*(NUM_CHANNELS - len(current_data_left))
    for rect, h in zip(bar_left, data_l):
        rect.set_height(h)

    data_r = current_data_right[:NUM_CHANNELS] + [0]*(NUM_CHANNELS - len(current_data_right))
    for rect, h in zip(bar_right, data_r):
        rect.set_height(h)
    return bar_left, bar_right

ani = animation.FuncAnimation(fig, update_plot, interval=50, blit=False, cache_frame_data=False)
plt.show()
