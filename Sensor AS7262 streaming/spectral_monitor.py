import serial
import serial.tools.list_ports
import time
import sys
import numpy as np
import scipy
import scipy.stats
from PyQt5 import QtCore, QtWidgets

from matplotlib.backends.backend_qt5agg import (
    FigureCanvas, NavigationToolbar2QT as NavigationToolbar)

from matplotlib.figure import Figure


value_col = ['V', 'B', 'G', 'Y', 'O', 'R']
colors = ['violet', 'blue', 'green', 'yellow', 'orange', 'red']
wavelenghts = [450, 500, 550, 570, 600, 650]

def plot_spectre(values, ax):
    for val, wl, color in zip(values, wavelenghts, colors):
        gauss = scipy.stats.norm(loc=wl, scale=20).pdf(np.arange(wl-50, wl+50))/(1/(20 * np.sqrt(2*np.pi)))
        ax.plot(np.arange(wl-50, wl+50),
                    gauss*val,
                color=color)
        #ax.set_ylim([0, 1000])


class ApplicationWindow(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self._main = QtWidgets.QWidget()
        self.setCentralWidget(self._main)
        self.setWindowTitle("Spectral Monitor")
        self.file = None

        layout = QtWidgets.QHBoxLayout(self._main)
        adafruit_ser_name = None
        while adafruit_ser_name is None:
            for ser in serial.tools.list_ports.comports(include_links=False):
                if (ser.vid == 9114 and ser.pid == 32780):
                    adafruit_ser_name = ser.device
        self.ser = serial.Serial(adafruit_ser_name, 9600, timeout=0)

        dynamic_canvas = FigureCanvas(Figure(figsize=(5, 3)))
        intensity_canvas = FigureCanvas(Figure(figsize=(3, 3)))

        values_grid   = QtWidgets.QGridLayout()
        values_gbox   = QtWidgets.QGroupBox("Values")
        values_labels = []
        self.values_edits  = []
        for id, val in enumerate(value_col):
            values_labels.append(QtWidgets.QLabel(val))
            self.values_edits.append(QtWidgets.QTextEdit('NaN'))
            self.values_edits[-1].setReadOnly(True)
            self.values_edits[-1].setFixedHeight(25)

            values_grid.addWidget(values_labels[-1], id, 0)
            values_grid.addWidget(self.values_edits[-1], id, 1)
            values_grid.setRowStretch(id, 0)
        
        values_grid.setRowStretch(id+1, 1)
        button = QtWidgets.QPushButton('Stream to file')
        button.clicked.connect(self.stream_to_file)
        values_grid.addWidget(button, id+2, 0)
        values_grid.setRowStretch(id+2, 0)

        values_gbox.setLayout(values_grid)

        layout.addWidget(dynamic_canvas, 3)
        layout.addWidget(intensity_canvas, 1)
        layout.addWidget(values_gbox, 1)


        self._dynamic_ax = dynamic_canvas.figure.subplots()

        self._intensity_ax = intensity_canvas.figure.subplots()
        self._intensity_ax.get_xaxis().set_visible(False)

        self._timer = dynamic_canvas.new_timer(
            0, [(self._update_canvas, (), {})])
        self._timer.start()

    def stream_to_file(self):
        fname = QtWidgets.QFileDialog.getSaveFileName(self, 'Save file',
            '', "Text File (*.txt)")
        if fname:
            self.file = open(fname[0], 'w')

    def _update_canvas(self):
        self._dynamic_ax.clear()
        line = self.ser.readline()
        values = line.decode("utf-8").strip().split(',')
        if len(values)<6:
            return
        values = [float(x[1:]) for x in values[1:]]
        plot_spectre(values, ax=self._dynamic_ax)
        self._dynamic_ax.figure.canvas.draw()

        self._intensity_ax.clear()
        self._intensity_ax.bar([0], np.log(np.sum(values)), width=0.1)
        self._intensity_ax.set_ylim([0, 15])
        self._intensity_ax.figure.canvas.draw()

        for val, edit in zip(values, self.values_edits):
            edit.setText("{}".format(val))

        if self.file is not None:
            self.file.write(time.strftime("%a, %d %b %Y %H:%M:%S,") + line.decode("utf-8"))

    def __exit__(self):
        self.ser.close()
        if self.file is not None:
            self.file.close()


if __name__ == "__main__":
    qapp = QtWidgets.QApplication(sys.argv)
    app = ApplicationWindow()
    app.show()
    qapp.exec_()