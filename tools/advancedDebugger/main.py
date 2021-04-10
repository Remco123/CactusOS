import serial
import asyncio

from typing import Iterator, Tuple
from serial.tools.list_ports import comports
from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtWidgets import QApplication, QMessageBox
from PyQt5.QtCore import QSettings
from PyQt5.QtGui import QCloseEvent
from quamash import QEventLoop
from numpy import *
import pyqtgraph as pg

import sys
import mainWindow
import copy

# Object for access to the serial port
ser = serial.Serial(timeout=0)
SER_BAUDRATE = 38400

# Setting constants
SETTING_PORT_NAME = 'port_name'
SETTING_DEBUG_FREQ = 'debug_freq'

connected = False
prevTime = float(0)

class PlotData:
    def __init__(self, targ, width):
        self.curve = targ
        self.Xm = linspace(0,0,width)
        self.ptr = -width
    
    def addItem(self, value):
        # shift data in the temporal mean 1 sample left
        self.Xm[:-1] = self.Xm[1:]
        # vector containing the instantaneous values
        self.Xm[-1] = value
        # update x position for displaying the curve
        self.ptr += 1
        # set the curve with this data
        self.curve.setData(self.Xm)
        # set x position in the graph to 0
        self.curve.setPos(self.ptr, 0)

def getSerialPorts():
    """Return all available serial ports."""
    ports = comports()
    return (p.device for p in ports)

def send_serial_async(msg: str) -> None:
    """Send a message to serial port (async)."""
    ser.write(msg.encode())

class ExampleApp(QtWidgets.QMainWindow, mainWindow.Ui_mainWindow):
    def __init__(self, parent=None):
        super(ExampleApp, self).__init__(parent)
        self.setupUi(self)

        # Setup events
        self.connectButton.pressed.connect(self.on_connect_btn_pressed)
        self.comPortSelect.currentIndexChanged.connect(self.onPortChanged)
        self.serialSendButton.pressed.connect(self.on_send_btn_pressed)

        self.updateComPorts()
        self.loadSettings()

        pg.setConfigOptions(antialias=True)

        # Setup graphs
        self.physMemoryGraph.enableAutoRange()
        self.physMemoryGraph.setLabels(left='Phys Memory Used [Mb]')
        self.physMemoryGraph.enableAutoRange(axis='y', enable=True)
        self.physMemoryGraph.setBackground('w')
        self.physMemoryData = PlotData(self.physMemoryGraph.plot(pen=pg.mkPen('b')), 200)

        self.heapMemoryGraph.enableAutoRange()
        self.heapMemoryGraph.setLabels(left='Heap Memory Used [Mb]')
        self.heapMemoryGraph.enableAutoRange(axis='y', enable=True)
        self.heapMemoryGraph.setBackground('w')
        self.heapMemoryData = PlotData(self.heapMemoryGraph.plot(pen=pg.mkPen('b')), 200)

        self.idleProcGraph.enableAutoRange()
        self.idleProcGraph.setLabels(left='Idle Process Iterations [I/s]')
        self.idleProcGraph.enableAutoRange(axis='y', enable=True)
        self.idleProcGraph.setBackground('w')
        self.idleProcData = PlotData(self.idleProcGraph.plot(pen=pg.mkPen('b')), 200)

        self.diskReadGraph.enableAutoRange()
        self.diskReadGraph.setLabels(left='Read Disk calls [C/s]')
        self.diskReadGraph.enableAutoRange(axis='y', enable=True)
        self.diskReadGraph.setBackground('w')
        self.diskReadData = PlotData(self.diskReadGraph.plot(pen=pg.mkPen('b')), 200)

        self.diskWriteGraph.enableAutoRange()
        self.diskWriteGraph.setLabels(left='Write Disk calls [C/s]')
        self.diskWriteGraph.enableAutoRange(axis='y', enable=True)
        self.diskWriteGraph.setBackground('w')
        self.diskWriteData = PlotData(self.diskWriteGraph.plot(pen=pg.mkPen('b')), 200)

        self.handleSerialMessage("[0] [Info] Welcome to the CactusOS Debugger application")
    def updateComPorts(self) -> None:
        """Update COM Port list in GUI."""
        for device in getSerialPorts():
            self.comPortSelect.addItem(device)

        for i in range(5):
            self.comPortSelect.addItem("/dev/pts/" + str(i))

    def loadSettings(self) -> None:
        """Load settings on startup."""
        settings = QSettings()

        # port name
        port_name = settings.value(SETTING_PORT_NAME)
        if port_name is not None:
            index = self.comPortSelect.findText(port_name)
            if index > -1:
                self.comPortSelect.setCurrentIndex(index)
        
        # debug frequency
        freq = settings.value(SETTING_DEBUG_FREQ)
        if freq is not None:
            self.refreshTimeBox.setValue(float(freq))

    def saveSettings(self) -> None:
        """Save settings on shutdown."""
        settings = QSettings()
        settings.setValue(SETTING_PORT_NAME, self.port)
        settings.setValue(SETTING_DEBUG_FREQ, self.refreshTimeBox.value())

    def closeEvent(self, event: QCloseEvent) -> None:
        """Handle Close event of the Widget."""
        if ser.is_open:
            ser.close()

        self.saveSettings()

        event.accept()

    def show_error_message(self, msg: str) -> None:
        """Show a Message Box with the error message."""
        QMessageBox.critical(self, QApplication.applicationName(), str(msg))

    def onPortChanged(self) -> None:
        self.port = self.comPortSelect.currentText()

    def on_connect_btn_pressed(self) -> None:
        """Open serial connection to the specified port."""
        if ser.is_open:
            ser.close()
        ser.port = self.port
        ser.baudrate = SER_BAUDRATE

        try:
            ser.open()
        except Exception as e:
            self.show_error_message(str(e))

        if ser.is_open:
            self.connectButton.setDisabled(True)
            self.comPortSelect.setDisabled(True)
            loop.create_task(self.MainLoop())

    def on_send_btn_pressed(self) -> None:
        """Send message to serial port."""
        msg = self.serialLogInput.text() + '\n'
        loop.call_soon(send_serial_async, msg)
        self.serialLogInput.clear()

    def handleSerialMessage(self, msg):
        global connected

        if msg[0] == '[':
            indx1 = msg.index("[", 1) + 1
            logType = msg[indx1:]
            logColor = "black"
            if logType.startswith("Info"):
                logColor = "black"
            elif logType.startswith("Warning"):
                logColor = "orange"
            elif logType.startswith("Error"):
                logColor = "red"

            self.serialLogOutput.insertHtml('<p style="pading: 0; margin: 0; color:' + logColor + '";>' + msg + '<br></p>')
        elif msg[0] == '$':
            print("Debug message: " + msg)
            
            if msg[1:] == "DebugReady":
                connected = True
            elif msg[1:].startswith("DebugSysSummary"):
                # Summary of system
                infoArray = msg[1:].split('|')
                
                infoMsg = """ <!DOCTYPE html>
                            <html>
                            <body>
                            
                            <h3>Kernel info:</h3>
                            <p>Built on: {0}</p>
                            
                            <h3>Operating Environment:</h3>
                            <p>Host: {1} {2}</p>
                            <p>Version: {3}</p>
                            
                            <h3>BIOS Environment:</h3>
                            <p>Manufacturer: {4}</p>
                            <p>Version: {5}</p>
                            
                            <h3>Runtime Environment:</h3>
                            <p>Installed Memory: {6} MB</p>
                            
                            </body>
                            </html>
                            """.format(infoArray[1], infoArray[2], infoArray[3], infoArray[4], infoArray[5], infoArray[6], int(infoArray[7]) / 1024.0 / 1024.0)
                
                self.generalInfoBox.setHtml(infoMsg)
            elif msg[1:].startswith("DebugUpdate"):
                infoArray = msg[1:].split('|')

                self.physMemoryData.addItem(int(infoArray[1]) / 1024.0 / 1024.0)
                self.heapMemoryData.addItem(int(infoArray[2]) / 1024.0 / 1024.0)
                self.idleProcData.addItem(int(infoArray[3]))
                self.diskReadData.addItem(int(infoArray[4]))
                self.diskWriteData.addItem(int(infoArray[5]))
        else:
            self.serialLogOutput.insertHtml('<p style="pading: 0; margin: 0; color:black">' + msg + '</p><br>')
            self.serialLogOutput.moveCursor(QtGui.QTextCursor.End)

    async def MainLoop(self) -> None:
        """Main application loop after connected to kernel"""
        global prevTime

        msg = ""
        while True:
            if(ser.is_open):
                data = ser.read()
                if data != b'':
                    try:
                        char = data.decode()
                        
                        if(char == '\n'):
                            msgCpy = copy.deepcopy(msg)
                            msg = ""
                            self.handleSerialMessage(msgCpy.strip())
                        else:
                            msg += char
                    except:
                        msg += '\0'
                
                newTime = loop.time()
                if connected and (newTime - prevTime >= self.refreshTimeBox.value()):
                    prevTime = newTime
                    loop.call_soon(send_serial_async, "ReqDebugUpdate\n")
            
            await asyncio.sleep(0)
    
if __name__ == '__main__':
    app = QApplication(sys.argv)

    loop = QEventLoop(app)
    asyncio.set_event_loop(loop)

    form = ExampleApp()
    form.show()

    with loop:
        loop.run_forever()
