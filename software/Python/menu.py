import tkinter as tk
from tkinter import ttk
import serial
import serial.tools.list_ports
from tkinter.messagebox import showinfo

class SerialControlGUI:
    def __init__(self, master):
        self.master = master
        master.title("Serial Direction Control")
        master.geometry("400x300")

        self.status = (0, 0)

        # Frame outer
        self.frame_up  = tk.Frame(master)
        self.frame_up.pack(side=tk.TOP)
        if True:
            self.port_label = tk.Label(self.frame_up, text="Select Serial Port:")
            self.port_label.pack(pady=5, padx=5)
        self.frame_up_bottom = tk.Frame(self.frame_up)
        self.frame_up_bottom.pack()
        if True:
            self.port_var = tk.StringVar()
            self.port_combobox = ttk.Combobox(self.frame_up_bottom, textvariable=self.port_var, state="readonly")
            self.port_combobox.pack(side=tk.LEFT, pady=5, padx=5)
            self.refresh_button = tk.Button(self.frame_up_bottom, text="Refresh Ports", command=self.refresh_ports)
            self.refresh_button.pack(side=tk.LEFT, pady=5, padx=5)
        if True:
            self.status_var = tk.StringVar()
            self.status_label = tk.Label(self.frame_up, textvariable=self.status_var)
            self.status_label.pack(pady=5, padx=5)
        
        # Initialize ports
        self.refresh_ports()
        
        self.frame_btm = tk.Frame(master)
        self.frame_btm.pack(side=tk.TOP, expand=True, fill=tk.BOTH)
        self.frame_btm_left = tk.Frame(self.frame_btm)
        self.frame_btm_left.pack(side=tk.LEFT, expand=True)
        if True:
            # create a list box
            langs = ('IR Module', 'NFC', 'WiFi Tools', 'Micro SD', 'Bluetooth',
                    'LoRA')
            self.menu_item = []
            self.menu_item.append(('Read IR', 'Send IR', 'List IR Cmd'))
            self.menu_item.append(('Read NFC'))
            self.menu_item.append(('Scan AP', 'Sniff Traffic'))
            self.menu_item.append(('List Directory'))
            self.menu_item.append(('Scan'))
            self.menu_item.append(('Receive', 'Send'))

            self.var = tk.Variable(value=langs)

            self.listbox = tk.Listbox(
                self.frame_btm_left,
                listvariable=self.var,
                height=8,
                selectmode=tk.SINGLE)

            self.listbox.pack(expand=True, fill=tk.BOTH, side=tk.LEFT)
            self.listbox.selection_clear(0, tk.END)
            self.listbox.selection_set(0)
            self.listbox.activate(0)

            # link a scrollbar to a list
            self.scrollbar = ttk.Scrollbar(
                self.frame_btm_left,
                orient=tk.VERTICAL,
                command=self.listbox.yview
            )

            self.listbox['yscrollcommand'] = self.scrollbar.set

            self.scrollbar.pack(side=tk.LEFT, expand=True, fill=tk.Y)
            self.listbox.bind('<<ListboxSelect>>', self.items_selected)

        self.frame_btm_right = tk.Frame(self.frame_btm)
        self.frame_btm_right.pack(side=tk.LEFT, expand=True)
        if True:
            n = 8
            self.button_up = tk.Button(self.frame_btm_right, text="Up", width=n)
            self.button_right = tk.Button(self.frame_btm_right, text="Right", width=n)
            self.button_down = tk.Button(self.frame_btm_right, text="Down", width=n)
            self.button_left = tk.Button(self.frame_btm_right, text="Left", width=n)
            self.button_ok = tk.Button(self.frame_btm_right, text="OK", width=int(n*0.8))
            self.button_cancel = tk.Button(self.frame_btm_right, text="Cancel", width=int(n*0.8))
            self.button_refresh = tk.Button(self.frame_btm_right, text="Refresh", width=int(n*0.8))

            self.button_up.grid(column=1, row=0, padx=5, pady=5)
            self.button_right.grid(column=2, row=1, padx=5, pady=5)
            self.button_down.grid(column=1, row=2, padx=5, pady=5)
            self.button_left.grid(column=0, row=1, padx=5, pady=5)
            self.button_ok.grid(column=1, row=1, padx=5, pady=5)
            self.button_cancel.grid(column=2, row=2, padx=5, pady=5)
            self.button_refresh.grid(column=0, row=2, padx=5, pady=5)

    def items_selected(self, event):
        # print(event)
        # get selected indices
        selected_indices = self.listbox.curselection()
        # get selected items
        selected_langs = ",".join([self.listbox.get(i) for i in selected_indices])
        msg = f'You selected: {selected_langs} {selected_indices}'

        self.listbox.selection_clear(0, tk.END)
        if selected_indices == "" or selected_indices == ():
            self.listbox.selection_set(0)
            self.listbox.activate(0)
            return
        showinfo(title='Information', message=msg)
        self.listbox.selection_set(selected_indices)
        self.listbox.activate(selected_indices)

    def refresh_ports(self):
        # Get available serial ports
        ports = [port.device for port in serial.tools.list_ports.comports()]
        
        # Update combobox
        self.port_combobox['values'] = ports
        
        if ports:
            self.port_combobox.set(ports[0])  # Set first port by default
            self.status_var.set(f"Found {len(ports)} port(s)")
        else:
            self.status_var.set("No ports found")

    def send_direction(self, direction):
        try:
            # Get selected port
            port = self.port_var.get()
            
            if not port:
                self.status_var.set("Select a port first!")
                return

            # Open serial connection
            ser = serial.Serial(port, baudrate=9600, timeout=1)
            
            # Send direction
            ser.write(f"[]{direction}\n".encode('utf-8'))
            
            # Update status
            self.status_var.set(f"Sent {direction} command")
            
            # Close serial connection
            ser.close()

        except serial.SerialException as e:
            self.status_var.set(f"Error: {str(e)}")
        except Exception as e:
            self.status_var.set(f"Unexpected error: {str(e)}")

        self.get_status()
    
    def get_status(self):
        try:
            # Get selected port
            port = self.port_var.get()
            
            if not port:
                self.status_var.set("Select a port first!")
                return

            # Open serial connection
            ser = serial.Serial(port, baudrate=9600, timeout=1)
            
            # Ask for status
            ser.write(b'get\n')

            # Wait for response
            data = [0, 0]
            while True:
                data[0] = ser.read()
                data[1] = ser.read()
                if data == b'[]':
                    break
            
            # Read status
            data = []
            while True:
                in_data = ser.read()
                if in_data == b'\n':
                    break
                else:
                    data.append(in_data)
                    self.status_var.set(data.decode('utf-8'))

            # Update status
            self.status = data.decode('utf-8').split(',')
            self.status = self.tuple(self.status)

            # Close serial connection
            ser.close()

        except serial.SerialException as e:
            self.status_var.set(f"Error: {str(e)}")
        except Exception as e:
            self.status_var.set(f"Unexpected error: {str(e)}")

def main():
    root = tk.Tk()
    gui = SerialControlGUI(root)
    root.mainloop()

if __name__ == "__main__":
    main()