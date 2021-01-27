import serial
import asyncio

from typing import Iterator, Tuple
from serial.tools.list_ports import comports
from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtWidgets import QApplication, QMessageBox
from PyQt5.QtCore import QSettings
from PyQt5.QtGui import QCloseEvent
from quamash import QEventLoop

import sys
import mainWindow

# Object for access to the serial port
ser = serial.Serial(timeout=0)
SER_BAUDRATE = 38400

# Setting constants
SETTING_PORT_NAME = 'port_name'

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

    def saveSettings(self) -> None:
        """Save settings on shutdown."""
        settings = QSettings()
        settings.setValue(SETTING_PORT_NAME, self.port)

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
        #print(self.port)

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
            loop.create_task(self.receive_serial_async())

    def on_send_btn_pressed(self) -> None:
        """Send message to serial port."""
        msg = self.serialLogInput.text() + '\r\n'
        loop.call_soon(send_serial_async, msg)
        self.serialLogInput.clear()

    def handleSerialMessage(self, msg):
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
            print("Debug message!")
        else:
            self.serialLogOutput.insertHtml('<p style="pading: 0; margin: 0; color:black">' + msg + '<br></p>')

        self.serialLogOutput.moveCursor(QtGui.QTextCursor.End)

    async def receive_serial_async(self) -> None:
        """Wait for incoming data, convert it to text and add to Textedit."""
        msg = ""
        while True:
            if(ser.is_open):
                data = ser.read()
                if data != b'':
                    try:
                        char = data.decode()
                        
                        if(char == '\n'):
                            self.handleSerialMessage(msg)
                            msg = ""
                        else:
                            msg += char
                    except:
                        msg += '\0'
            
            await asyncio.sleep(0)

    
if __name__ == '__main__':
    app = QApplication(sys.argv)

    loop = QEventLoop()
    asyncio.set_event_loop(loop)

    form = ExampleApp()
    form.show()

    with loop:
        loop.run_forever()