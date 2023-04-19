import string
import tkinter as tk
from queue import Queue
from tkinter import ttk
import argparse
import time

from can_constants import *
from socketcan import CanRawSocket, CanFrame, CanFdFrame
import can

import threading


def read_can_raw():
    interface = "vcan0"
    s = CanRawSocket(interface=interface)

    while True:
        frame = s.recv()

        row = "ID = {0:8X} {1}  FRAME_LEN = [{2}] FRAME_DATA = {3}".format(
            frame.can_id,
            get_message_from_id(frame.can_id),
            len(frame.data),
            " ".join(["{0:02X}".format(b) for b in frame.data])
        )
        print(row)


class GUI:
    def __init__(self, can_interface: string, using_obd2: bool):
        self.root = tk.Tk()
        self.data_queue = Queue()
        self.can_interface = can_interface
        self.using_obd2 = using_obd2
        self.selected_frame = {
            "id": tk.StringVar(),
            "data": tk.StringVar()
        }
        self.replay_number_of_injections = tk.StringVar()
        self.replay_interval = tk.StringVar()
        self.replay_interval_options = ['2 Hz', '10 Hz', '20 Hz', '100 Hz']
        self.counter = 0
        self.reader = None
        self.injector = None
        self.injection_progressbar = None
        self.inject_button = None
        self.tree, self.filter_checkboxes, self.selected_all, self.notebook = self.create_gui()

    def inject_frame(self, button: tk.Button, can_id: string, data: string, frequency: string,
                     number_of_injections: string):

        if button.do_cancel:
            self.injector.stopped = True
            self.join_injection()
            self.injection_progressbar.stop()

        else:
            id_conv = int(can_id, 16)
            data_conv = bytearray.fromhex(data.replace(' ', ''))
            frequency_conv = int(frequency.replace(' Hz', ''))
            number_of_injections = int(number_of_injections)

            self.injector = Injector(can_interface=self.can_interface, can_id=id_conv, data=data_conv,
                                     frequency=frequency_conv, number_of_injections=number_of_injections)

            button.do_cancel = True
            self.injector.start()

            self.root.after(int((1000 / frequency_conv)) * number_of_injections, self.join_injection)

            self.injection_progressbar['maximum'] = number_of_injections
            interval_millisecond = int((1000 / frequency_conv))

            self.injection_progressbar.start(interval=interval_millisecond)
            button.configure(text='Cancel Injection')

    def join_injection(self):
        self.injector.join()
        self.injection_progressbar.stop()
        self.inject_button.configure(text='Inject Frame')
        self.inject_button.do_cancel = False

    def restart_reader(self):
        self.stop_reader()
        self.start_reader()

    def start_reader(self):
        checkbox_values_raw = {}

        for key, value in self.filter_checkboxes.items():
            checkbox_values_raw[key] = value.get()

        self.reader = Reader(can_id_filter=checkbox_values_raw, can_interface=self.can_interface,
                             counter=self.counter, data_queue=self.data_queue)
        self.reader.start()

    def stop_reader(self):
        self.reader.stopped = True
        self.reader.join()
        self.counter = self.reader.counter

    def show(self):
        self.start_reader()
        self.check_for_data()
        self.root.mainloop()

    def check_for_data(self):
        for cnt in range(100):
            try:
                row = self.data_queue.get(block=False)
                self.tree.insert("", 0, text="L1", values=row)

            except Exception as e:
                pass
        self.root.after(50, self.check_for_data)

    def stop_sniffing(self, button: tk.Button):
        if button.isRunning:
            self.stop_reader()
            button.configure(bg='grey', text='Stopped')

        else:
            self.start_reader()
            button.configure(bg='green', text='Running')

        button.isRunning = not button.isRunning

    def send_to_replay(self):
        self.notebook.select(1)
        pass

    def select_all(self):
        new_value = self.selected_all.get()
        for checkbox in self.filter_checkboxes.values():
            checkbox.set(new_value)

        for key, value in self.filter_checkboxes.items():
            self.reader.set_filter(key, new_value)

    def on_change_filter_value(self, can_id):
        new_value = self.filter_checkboxes.get(can_id).get()
        self.selected_all.set(False)
        self.reader.set_filter(can_id, new_value)

    def clear_tree(self):
        for item in self.tree.get_children():
            self.tree.delete(item)

    def show_full_data(self, button: tk.Button):
        if button.isFullData:
            self.tree["displaycolumns"] = ('#', 'id', 'id meaning', 'length', 'data_preview')
            button.configure(bg='grey')
        else:
            self.tree["displaycolumns"] = ('#', 'id', 'id meaning', 'length', 'data_full')
            button.configure(bg='green')

        button.isFullData = not button.isFullData

    def create_sniff_tab(self, notebook: ttk.Notebook):
        sniffing_frame = ttk.Frame(notebook)
        sniffing_frame.pack(fill='both', expand=True)

        # splitting the window in left and right
        left_frame = tk.Frame(sniffing_frame)
        left_frame.pack(fill="both", side=tk.LEFT, padx=10, pady=10, expand=True)
        right_frame = tk.Frame(sniffing_frame)
        right_frame.pack(side=tk.LEFT, padx=10, pady=10, expand=True, fill=tk.BOTH)

        # add section for sniffing settings on the top left
        settings_label_frame = ttk.LabelFrame(left_frame, text='Sniffing Settings')
        settings_label_frame.pack(fill="both", expand=False, padx=(10, 10), pady=(5, 5))

        start_stop_button = tk.Button(
            settings_label_frame, bg='green', text='Running', command=lambda: self.stop_sniffing(start_stop_button))
        start_stop_button.isRunning = True
        start_stop_button.grid(row=0, column=0, sticky="ew", padx=(10, 10), pady=(10, 10))

        show_full_data_button = tk.Button(
            settings_label_frame, bg='grey', text="Show Full Data",
            command=lambda: self.show_full_data(show_full_data_button))
        show_full_data_button.isFullData = False
        show_full_data_button.grid(row=1, column=0, sticky="ew", padx=(10, 10), pady=(10, 10))

        clear_data_button = tk.Button(settings_label_frame, text="clear data", command=self.clear_tree)
        clear_data_button.grid(row=1, column=1, sticky="ew", padx=(10, 10), pady=(10, 10))

        send_to_replay_button = tk.Button(
            settings_label_frame, text="Send to Replay", command=self.send_to_replay, state=tk.DISABLED)
        send_to_replay_button.grid(row=0, column=1, sticky="ew", padx=(10, 10), pady=(10, 10))

        # add section for filtering can packages on the bottom left
        filter_label_frame = ttk.LabelFrame(left_frame, text='Filter')
        filter_label_frame.pack(fill="both", expand=True, padx=(10, 10), pady=(5, 5))

        filter_canvas = tk.Canvas(filter_label_frame)
        filter_canvas.pack(side='left', fill='both', expand=True)

        filter_inner_frame = tk.Frame(filter_canvas)
        filter_inner_frame.pack(side="top", fill="both", expand=True)

        checkbox_values = {}
        selected_all = tk.BooleanVar()

        checkbox = tk.Checkbutton(filter_inner_frame, text="all", variable=selected_all, command=self.select_all)
        checkbox.pack(side="top", anchor="w")

        for key, value in can_messages_dict.items():
            var = tk.BooleanVar()
            checkbox = tk.Checkbutton(
                filter_inner_frame, text=value, variable=var,
                command=lambda k=key: self.on_change_filter_value(k))
            checkbox.pack(side="top", anchor="w", padx=15)
            checkbox_values[key] = var

        checkbox_frame_scrollbar = tk.Scrollbar(filter_label_frame, orient="vertical", command=filter_canvas.yview)
        checkbox_frame_scrollbar.pack(side="right", fill="y")

        filter_canvas.configure(yscrollcommand=checkbox_frame_scrollbar.set)

        filter_canvas.create_window((0, 0), window=filter_inner_frame, anchor="nw")
        filter_inner_frame.update_idletasks()
        filter_canvas.configure(scrollregion=filter_canvas.bbox("all"))

        tree = ttk.Treeview(right_frame, columns=('#', 'id', 'id meaning', 'length', 'data_preview', 'data_full'),
                            show='headings')
        tree.heading('#', text='#')
        # tree.column("#", minwidth=0, width=100)
        tree.heading('id', text='id')
        tree.heading('id meaning', text='id meaning')
        tree.heading('length', text='length')
        tree.heading('data_preview', text='data (preview)')
        tree.heading('data_full', text='data (full)')
        tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, pady=(10, 0))
        tree["displaycolumns"] = ('#', 'id', 'id meaning', 'length', 'data_preview')

        def select_can_frame(item):
            send_to_replay_button['state'] = tk.NORMAL
            selected_can_frame_raw = tree.item(tree.focus())['values']
            self.selected_frame.get('id').set(selected_can_frame_raw[1].replace('0x', ''))
            self.selected_frame.get('data').set(selected_can_frame_raw[5])

        tree.bind("<<TreeviewSelect>>", select_can_frame)

        tree_y_scrollbar = tk.Scrollbar(right_frame, orient="vertical")
        tree_y_scrollbar.config(command=tree.yview)
        tree.configure(yscrollcommand=tree_y_scrollbar.set)
        tree_y_scrollbar.pack(side=tk.RIGHT, fill=tk.BOTH, pady=(10, 0))

        return tree, checkbox_values, selected_all, sniffing_frame

    def create_replay_tab(self, notebook: ttk.Notebook):
        replay_frame = ttk.Frame(notebook)
        replay_frame.pack(fill='both', expand=True)

        def validate_hex_input(new_value):
            valid = all(c in '0123456789abcdefABCDEF ' for c in new_value)
            return valid

        validate_command = (self.root.register(validate_hex_input), '%P')

        replay_frame_row_cnt = 1

        can_frame_label_frame = ttk.LabelFrame(replay_frame, text='Frame to Replay')
        can_frame_label_frame.grid(row=replay_frame_row_cnt, column=0, sticky="ew", padx=(20, 20), pady=(20, 20))
        replay_frame_row_cnt += 1

        replay_can_frame_frame_cnt = 1
        label_id = tk.Label(can_frame_label_frame, text='id:     0x')
        label_id.grid(row=replay_can_frame_frame_cnt, column=0, sticky='e', padx=(20, 1), pady=(20, 10))
        entry_id = tk.Entry(can_frame_label_frame, textvariable=self.selected_frame['id'],
                            validate='key', validatecommand=validate_command)
        entry_id.grid(row=replay_can_frame_frame_cnt, column=1, sticky='e', padx=(1, 10), pady=(20, 10))
        replay_can_frame_frame_cnt += 1

        label_data = tk.Label(can_frame_label_frame, text='data:     0x')
        label_data.grid(row=replay_can_frame_frame_cnt, column=0, sticky='e', padx=(20, 1), pady=(5, 10))
        entry_data = tk.Entry(can_frame_label_frame, textvariable=self.selected_frame['data'],
                              validate='key', validatecommand=validate_command)
        entry_data.grid(row=replay_can_frame_frame_cnt, column=1, sticky='e', padx=(1, 10), pady=(5, 10))
        replay_frame_row_cnt += 1

        replay_stetting_label_frame = ttk.LabelFrame(replay_frame, text='Replay Settings')
        replay_stetting_label_frame.grid(row=replay_frame_row_cnt, column=0, sticky="ew", padx=(20, 20), pady=(20, 20))
        replay_inject_input_frame_row_cnt = 1

        label_data = tk.Label(replay_stetting_label_frame, text='Injections Frequency')
        label_data.grid(row=replay_inject_input_frame_row_cnt, column=0, sticky='w', padx=(20, 1), pady=(5, 5))

        replay_interval_combobox = ttk.Combobox(replay_stetting_label_frame, state="readonly",
                                                textvariable=self.replay_interval, values=self.replay_interval_options)
        replay_interval_combobox.current(len(self.replay_interval_options) - 1)
        replay_interval_combobox.grid(row=replay_inject_input_frame_row_cnt,
                                      column=1, sticky='e', padx=(1, 10), pady=(5, 5))
        replay_inject_input_frame_row_cnt += 1

        label_data = tk.Label(replay_stetting_label_frame, text='Number of Injections')
        label_data.grid(row=replay_inject_input_frame_row_cnt, column=0, sticky='w', padx=(20, 1), pady=(5, 5))

        def validate_int_input(new_value):
            return new_value.isdigit() and 1000 >= int(new_value) > 0

        validate_command = (self.root.register(validate_int_input), '%P')
        replay_number_of_injections_spinner = tk.Spinbox(replay_stetting_label_frame,
                                                         from_=1, to=1000,
                                                         textvariable=self.replay_number_of_injections,
                                                         validate='key', validatecommand=validate_command)
        replay_number_of_injections_spinner.grid(row=replay_inject_input_frame_row_cnt,
                                                 column=1, sticky='e', padx=(1, 10), pady=(5, 10))
        replay_frame_row_cnt += 1

        self.injection_progressbar = ttk.Progressbar(
            replay_frame, style="Horizontal.TProgressbar", orient='horizontal', mode='determinate')
        self.injection_progressbar.grid(row=replay_frame_row_cnt, column=0, sticky='we', padx=(20, 20))
        replay_frame_row_cnt += 1

        self.inject_button = tk.Button(replay_frame, text="Inject Frame", bg="red",
                                       command=lambda: self.inject_frame(
                                           self.inject_button,
                                           self.selected_frame.get('id').get(),
                                           self.selected_frame.get('data').get(),
                                           self.replay_interval.get(),
                                           self.replay_number_of_injections.get()))
        self.inject_button.do_cancel = False
        self.inject_button.grid(row=replay_frame_row_cnt, column=0, sticky='e', padx=(20, 20), pady=(20, 20))
        return replay_frame

    def create_gui(self):
        style = ttk.Style(self.root)
        style.theme_use('clam')
        style.configure("Horizontal.TProgressbar", background='red')

        self.root.title("Sniffing and Replay")
        self.root.minsize(600, 200)

        top_frame = tk.Frame(self.root)
        top_frame.pack(side=tk.TOP)

        subtitle = "Reading/Writing via OBD2 port (" + self.can_interface + ")" if self.using_obd2 else \
            "Reading/Writing direct on CAN bus via clipping (" + self.can_interface + ")"

        subtitle_label = tk.Label(top_frame, text=subtitle)
        subtitle_label.pack(side=tk.LEFT, expand=False, pady=10)

        bottom_frame = tk.Frame(self.root)
        bottom_frame.pack(side=tk.BOTTOM, expand=True, fill=tk.BOTH)

        notebook = ttk.Notebook(bottom_frame)
        notebook.pack(expand=True, fill='both')

        tree, checkbox_values, selected_all, sniffing_tab = self.create_sniff_tab(notebook)
        notebook.add(sniffing_tab, text='Sniffing')

        replay_frame = self.create_replay_tab(notebook)
        notebook.add(replay_frame, text='Replay')

        return tree, checkbox_values, selected_all, notebook


class Injector(threading.Thread):
    def __init__(self, can_id: int, data: bytearray, frequency: int, can_interface: string, number_of_injections: int):
        threading.Thread.__init__(self, daemon=True)
        self.stopped = False
        self.can_id = can_id
        self.data = data
        self.frequency = frequency
        self.can_raw_socket = CanRawSocket(interface=can_interface)
        self.number_of_injections = number_of_injections

    def run(self) -> None:
        frame = None
        if len(self.data) < 8:
            frame = CanFrame(can_id=self.can_id, data=self.data)
        else:
            frame = CanFdFrame(can_id=self.can_id, data=self.data)

        sleep_time = 1 / self.frequency
        for i in range(self.number_of_injections):
            if self.stopped:
                return
            self.can_raw_socket.send(frame)
            time.sleep(sleep_time)


class Reader(threading.Thread):
    def __init__(self, can_id_filter, can_interface: string, counter: int, data_queue: Queue):
        threading.Thread.__init__(self, daemon=True)
        self.stopped = False
        self.can_id_filter = can_id_filter
        self.counter = counter
        self.can_interface = can_interface
        self.data_queue = data_queue
        self.can_raw_socket = CanRawSocket(interface=self.can_interface)
        # self.can_raw_socket = can.Bus(channel=self.can_interface, interface="socketcan")

    def run(self) -> None:

        while not self.stopped:
            frame = self.can_raw_socket.recv()
            if frame is None:
                continue

            can_id = frame.can_id
            data = frame.data

            if self.can_id_filter[frame.can_id]:
                self.counter += 1
                data_full = " ".join(["{0:02X}".format(b) for b in data])
                data_preview = data_full[:5]
                row = (self.counter, hex(can_id), get_message_from_id(can_id), len(data),
                       data_preview, data_full)
                # queue is threadsafe: https://docs.python.org/3/library/queue.html
                self.data_queue.put(row)

    def set_filter(self, can_id, value):
        self.can_id_filter[can_id] = value


def main(can_interface: string, using_obd2: bool):
    gui = GUI(can_interface=can_interface, using_obd2=using_obd2)
    gui.show()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        prog='SniffingTable',
        description='Listens on the given CAN interface and gives the functionality to replay sniffed frames.')

    parser.add_argument(
        '--interface', default="vcan0",
        help='interface which should be used for sniffing (default: vcan0)')
    parser.add_argument(
        '--use-odb2', action='store_true',
        help='Indicates if the sniffer is clipped directly on the interface or the ODB2 port is used '
             '(changes only some text output)')

    args = parser.parse_args()

    main(args.interface, args.use_odb2)
