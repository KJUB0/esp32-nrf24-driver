import matplotlib
# If the window STILL doesn't open, change 'TkAgg' to 'Qt5Agg' or comment this line out
matplotlib.use('TkAgg') 
import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import threading
import time
import numpy as np

# --- CONFIGURATION ---
SERIAL_PORT = '/dev/ttyUSB0' 
BAUD_RATE = 115200
NUM_CHANNELS = 40
THRESHOLD = 9 

# --- SETUP SERIAL ---
print("Attempting to connect to serial...")
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
    print(f"Connected to {SERIAL_PORT} at {BAUD_RATE} baud.")
except Exception as e:
    print(f"Error connecting to serial port: {e}")
    print("TIP: Is the ESP32 plugged in? Is another program (like idf.py monitor) using the port?")
    exit()

# Global variables for data
current_data_left = [0] * NUM_CHANNELS
current_data_right = [0] * NUM_CHANNELS

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
            time.sleep(0.01)

thread = threading.Thread(target=read_serial, daemon=True)
thread.start()

# --- PLOT SETUP ---
print("Initializing GUI window...")
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))
fig.suptitle('NRF24 Spectrum Analyzer (Wi-Fi Filtered)')

bar_left = ax1.bar(range(NUM_CHANNELS), [0]*NUM_CHANNELS, color='cyan')
ax1.set_ylim(0, 15)
ax1.set_title("Left Radio")

bar_right = ax2.bar(range(NUM_CHANNELS), [0]*NUM_CHANNELS, color='magenta')
ax2.set_ylim(0, 15)
ax2.set_title("Right Radio")

SENSITIVITY = 4       # How many units ABOVE the noise floor a spike must be
WIFI_WIDTH_LIMIT = 5  # If more than 5 adjacent channels are high, it's WiFi

def get_active_threshold(data):
    """Calculates a baseline noise floor dynamically."""
    if not data: return 15
    # Use the median to ignore occasional high spikes when calculating the floor
    return np.median(data) + SENSITIVITY

def update_plot(frame):
    # Prepare data
    data_l = current_data_left[:NUM_CHANNELS] + [0]*(NUM_CHANNELS - len(current_data_left))
    data_r = current_data_right[:NUM_CHANNELS] + [0]*(NUM_CHANNELS - len(current_data_right))
    
    # Calculate Dynamic Thresholds
    thresh_l = get_active_threshold(data_l)
    thresh_r = get_active_threshold(data_r)

    def process_radio(data, bars, threshold):
        drone_detected = False
        num_pts = len(data)
        
        for i in range(num_pts):
            val = data[i]
            bars[i].set_height(val)
            
            if val > threshold:
                # --- PROPER WIFI FILTER ---
                # Check a window around the current point
                start = max(0, i - 2)
                end = min(num_pts, i + 3)
                nearby_high_channels = sum(1 for x in data[start:end] if x > (threshold - 2))
                
                if nearby_high_channels >= WIFI_WIDTH_LIMIT:
                    bars[i].set_color('lightgray') # Broad signal = WiFi
                else:
                    bars[i].set_color('red')       # Narrow signal = Drone!
                    drone_detected = True
            else:
                # Color based on radio (Cyan for Left, Magenta for Right)
                bars[i].set_color('cyan' if bars == bar_left else 'magenta')
        
        return drone_detected

    drone_l = process_radio(data_l, bar_left, thresh_l)
    drone_r = process_radio(data_r, bar_right, thresh_r)

    # Update Titles with live noise floor info
    ax1.set_title(f"LEFT (0-40): {'!!! DRONE !!!' if drone_l else 'Clear'} | Floor: {thresh_l:.1f}")
    ax2.set_title(f"RIGHT (40-80): {'!!! DRONE !!!' if drone_r else 'Clear'} | Floor: {thresh_r:.1f}")
    
    return bar_left, bar_right

ani = animation.FuncAnimation(fig, update_plot, interval=50, blit=False, cache_frame_data=False)
print("Opening Window... (If this hangs, check your terminal for errors)")
plt.show()