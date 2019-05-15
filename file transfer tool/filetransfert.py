import serial
import serial.tools.list_ports
import time
import sys
import re
from PyQt5 import QtCore, QtWidgets


class ApplicationWindow(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self._main = QtWidgets.QWidget()
        self.setCentralWidget(self._main)
        self.setWindowTitle("Adalogger transfert tool")
        layout = QtWidgets.QVBoxLayout(self._main)

        self.text = QtWidgets.QLabel(
            'Unplug the DataLogger from the computer and remove the battery')
        self.log = QtWidgets.QStatusBar()
        self.setStatusBar(self.log)
        self.button = QtWidgets.QPushButton('Next')
        self.button.clicked.connect(self.next)
        self.list = QtWidgets.QListWidget()
        self.list.itemClicked.connect(self.listClicked)
        self.list.setVisible(False)

        layout.addWidget(self.text, 0)
        layout.addWidget(self.button, 0)
        layout.addWidget(self.list, 1)

    def next(self):
        self.text.setText('Plug it in the computer')
        self.log.setVisible(True)
        self.button.setVisible(False)
        self.connect_to_datalogger()

    def listClicked(self, e):
        file, size = e.text().split('\t')
        fname = QtWidgets.QFileDialog.getSaveFileName(self, 'Save file',
                                                      'c:\\'+file, "Text File (*.txt)")
        self.list.setEnabled(False)
        if fname:
            self.reveive_file(file, int(size), fname[0])
        self.list.setEnabled(True)

    def __exit__(self):
        self.adafruit_ser.close()

    def log_text(self, text):
        print(text)
        self.log.showMessage(text)
        QtCore.QCoreApplication.processEvents()

    def connect_to_datalogger(self):
        adafruit_ser_name = None
        while adafruit_ser_name is None:
            self.log_text("looking for adafruit adalogger")

            for ser in serial.tools.list_ports.comports(include_links=False):
                if (ser.vid == 9114 and ser.pid == 32780):
                    self.log_text("Found adafruit adalogger")
                    adafruit_ser_name = ser.device

        self.adafruit_ser = serial.Serial(adafruit_ser_name, 9600, timeout=2)
        while not self.adafruit_ser.is_open:
            self.log_text("Trying to connect")
            self.adafruit_ser = serial.Serial(
                adafruit_ser_name, 9600, timeout=2)

        while True:
            line = self.adafruit_ser.readline()
            if line:
                line = line.decode("utf8", "ignore")
                self.log_text(line)
                if "Waiting for PC" in line:
                    self.adafruit_ser.write(bytes(1))
                    break

        lines = []
        while True:
            line = self.adafruit_ser.readline()
            if line:
                line = line.decode("utf8", "ignore")
                lines.append(line)
                self.log_text(line)
                if "Waiting for choice" in line:
                    choices = [re.search('(\d+.TXT)\t+(\d+)', line, re.IGNORECASE).groups()
                               for line in lines if re.search('(\d+.TXT)\t+(\d+)', line, re.IGNORECASE)]

                    for choice in choices:
                        self.list.addItem("{}\t{}".format(*choice))
                    self.list.setVisible(True)
                    return

    def reveive_file(self, file, size, fname):
        self.adafruit_ser.write(file.encode('ascii'))

        while True:
            line = self.adafruit_ser.readline()
            if line:
                line = line.decode("utf8", "ignore")
                if "Prepare Sending File" in line:
                    self.log_text(line)
                    break
        recv_bytes = bytes()
        with open(fname, 'wb') as f:
            while b'Done' not in recv_bytes:
                self.log_text(
                    'Receiving ' + "{:.0%}".format(len(recv_bytes)/size) + '.'*(len(recv_bytes) % 4))
                QtCore.QCoreApplication.processEvents()
                byte = self.adafruit_ser.readline()
                recv_bytes += byte
                f.write(bytes(byte))
        self.log_text('Done!')


if __name__ == "__main__":
    try:
        qapp = QtWidgets.QApplication(sys.argv)
        app = ApplicationWindow()
        app.show()
        qapp.exec_()
    finally:
        app.adafruit_ser.close()
