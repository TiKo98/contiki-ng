"""
Script to show how visualization could happen.
Two threads are working parallel. One thred is producing data, the other one is visualizing it.
This is used to simulate incoming data from outside the program.
"""
# For Visuals
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
# For Threading
import threading
import time

import dummy_data_functions as ddf

class BluetoothData:
    """Handles all data that needs to be excahnged between Threads"""
    def __init__(self):
        self.data = []
        self._lock = threading.Lock()

    def add_data(self, data):
        """Ads new data"""
        with self._lock:
            local_copy = self.data
            local_copy.append(data)
            self.data = local_copy

    def read_data(self):
        """Yields last added value and deletes it (FIFO), keeps lock until data is empty."""
        with self._lock:
            while(len(self.data) > 0):
                val = self.data[0]
                self.data = self.data[1:]
                yield val

    def read_all_data(self):
        """Returns all data currently saved, empties data list"""
        with self._lock:
            val = self.data
            self.data = []
            return val

def receive_data(): # data as argument?
    """simulates receiving new Data every 20ms"""
    global db
    while True:
        current_time = round(time.time() * 1000)
        acc = ddf.realistic_acceleration(current_time)
        # db.add_data([current_time, acc])
        if acc > 0:     
            db.add_data([current_time, acc])
        else:
            # if the acceleration is negative the chip doesn't send any data
            db.add_data([-999, -999])
        time.sleep(0.02)
    

def animate_data(i):
    """reads data and shows it strokewise"""
    global db, stroke_x, stroke_y

    new_vals = db.read_all_data()
    # new_vals = [[x, 8] for x in range(20)]
    
    # new_vals = []
    # for v in db.read_data():
    #     new_vals.append(v)

    for e in new_vals:
        if e != [-999, -999]:
            stroke_x.append(e[0] % 3000)
            stroke_y.append(e[1])
        else:
            stroke_x = []
            stroke_y = []
            plt.clf()
    
    # plt.style.use("ggplot")
    plt.plot(stroke_x, stroke_y)


#####################################################################
#                               main                                #
#####################################################################
fig = plt.figure(figsize=(8,8))
db = BluetoothData()
stroke_x, stroke_y = [], []

t = threading.Thread(target=receive_data, args=(), daemon=True)
t.start()
ani = FuncAnimation(fig, animate_data, interval=20)
plt.show()