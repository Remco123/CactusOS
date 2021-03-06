# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'tools/advancedDebugger/mainGUI.ui'
#
# Created by: PyQt5 UI code generator 5.14.1
#
# WARNING! All changes made in this file will be lost!


from PyQt5 import QtCore, QtGui, QtWidgets


class Ui_mainWindow(object):
    def setupUi(self, mainWindow):
        mainWindow.setObjectName("mainWindow")
        mainWindow.resize(900, 700)
        sizePolicy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(mainWindow.sizePolicy().hasHeightForWidth())
        mainWindow.setSizePolicy(sizePolicy)
        self.centralwidget = QtWidgets.QWidget(mainWindow)
        self.centralwidget.setObjectName("centralwidget")
        self.groupBox = QtWidgets.QGroupBox(self.centralwidget)
        self.groupBox.setGeometry(QtCore.QRect(10, 80, 421, 611))
        sizePolicy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.groupBox.sizePolicy().hasHeightForWidth())
        self.groupBox.setSizePolicy(sizePolicy)
        self.groupBox.setAlignment(QtCore.Qt.AlignBottom|QtCore.Qt.AlignLeading|QtCore.Qt.AlignLeft)
        self.groupBox.setObjectName("groupBox")
        self.serialSendButton = QtWidgets.QPushButton(self.groupBox)
        self.serialSendButton.setGeometry(QtCore.QRect(320, 580, 89, 25))
        self.serialSendButton.setObjectName("serialSendButton")
        self.serialLogOutput = QtWidgets.QTextEdit(self.groupBox)
        self.serialLogOutput.setGeometry(QtCore.QRect(0, 20, 421, 551))
        font = QtGui.QFont()
        font.setPointSize(9)
        self.serialLogOutput.setFont(font)
        self.serialLogOutput.setLineWrapMode(QtWidgets.QTextEdit.NoWrap)
        self.serialLogOutput.setReadOnly(True)
        self.serialLogOutput.setObjectName("serialLogOutput")
        self.serialLogInput = QtWidgets.QLineEdit(self.groupBox)
        self.serialLogInput.setGeometry(QtCore.QRect(10, 580, 291, 25))
        self.serialLogInput.setObjectName("serialLogInput")
        self.groupBox_2 = QtWidgets.QGroupBox(self.centralwidget)
        self.groupBox_2.setGeometry(QtCore.QRect(10, 0, 881, 71))
        self.groupBox_2.setObjectName("groupBox_2")
        self.comPortSelect = QtWidgets.QComboBox(self.groupBox_2)
        self.comPortSelect.setGeometry(QtCore.QRect(90, 34, 681, 25))
        self.comPortSelect.setObjectName("comPortSelect")
        self.connectButton = QtWidgets.QPushButton(self.groupBox_2)
        self.connectButton.setGeometry(QtCore.QRect(780, 34, 89, 25))
        self.connectButton.setObjectName("connectButton")
        self.label = QtWidgets.QLabel(self.groupBox_2)
        self.label.setGeometry(QtCore.QRect(10, 37, 91, 17))
        self.label.setObjectName("label")
        self.groupBox_3 = QtWidgets.QGroupBox(self.centralwidget)
        self.groupBox_3.setGeometry(QtCore.QRect(440, 80, 451, 611))
        self.groupBox_3.setObjectName("groupBox_3")
        self.tabWidget = QtWidgets.QTabWidget(self.groupBox_3)
        self.tabWidget.setGeometry(QtCore.QRect(0, 20, 451, 551))
        sizePolicy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Preferred, QtWidgets.QSizePolicy.Preferred)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.tabWidget.sizePolicy().hasHeightForWidth())
        self.tabWidget.setSizePolicy(sizePolicy)
        self.tabWidget.setObjectName("tabWidget")
        self.tab_General = QtWidgets.QWidget()
        self.tab_General.setObjectName("tab_General")
        self.generalInfoBox = QtWidgets.QTextEdit(self.tab_General)
        self.generalInfoBox.setGeometry(QtCore.QRect(8, 8, 431, 501))
        self.generalInfoBox.setLineWrapMode(QtWidgets.QTextEdit.NoWrap)
        self.generalInfoBox.setReadOnly(True)
        self.generalInfoBox.setObjectName("generalInfoBox")
        self.tabWidget.addTab(self.tab_General, "")
        self.tab_CPU = QtWidgets.QWidget()
        self.tab_CPU.setObjectName("tab_CPU")
        self.idleProcGraph = PlotWidget(self.tab_CPU)
        self.idleProcGraph.setGeometry(QtCore.QRect(10, 10, 429, 247))
        self.idleProcGraph.setAutoFillBackground(True)
        self.idleProcGraph.setObjectName("idleProcGraph")
        self.cpuInfoBox = QtWidgets.QTextEdit(self.tab_CPU)
        self.cpuInfoBox.setGeometry(QtCore.QRect(10, 270, 431, 241))
        self.cpuInfoBox.setLineWrapMode(QtWidgets.QTextEdit.NoWrap)
        self.cpuInfoBox.setReadOnly(True)
        self.cpuInfoBox.setObjectName("cpuInfoBox")
        self.tabWidget.addTab(self.tab_CPU, "")
        self.tab_Disk = QtWidgets.QWidget()
        self.tab_Disk.setObjectName("tab_Disk")
        self.widget = QtWidgets.QWidget(self.tab_Disk)
        self.widget.setGeometry(QtCore.QRect(10, 10, 431, 501))
        self.widget.setObjectName("widget")
        self.verticalLayout_2 = QtWidgets.QVBoxLayout(self.widget)
        self.verticalLayout_2.setContentsMargins(0, 0, 0, 0)
        self.verticalLayout_2.setObjectName("verticalLayout_2")
        self.diskReadGraph = PlotWidget(self.widget)
        self.diskReadGraph.setAutoFillBackground(True)
        self.diskReadGraph.setObjectName("diskReadGraph")
        self.verticalLayout_2.addWidget(self.diskReadGraph)
        self.diskWriteGraph = PlotWidget(self.widget)
        self.diskWriteGraph.setAutoFillBackground(True)
        self.diskWriteGraph.setObjectName("diskWriteGraph")
        self.verticalLayout_2.addWidget(self.diskWriteGraph)
        self.tabWidget.addTab(self.tab_Disk, "")
        self.tab_Memory = QtWidgets.QWidget()
        self.tab_Memory.setObjectName("tab_Memory")
        self.layoutWidget = QtWidgets.QWidget(self.tab_Memory)
        self.layoutWidget.setGeometry(QtCore.QRect(10, 10, 431, 501))
        self.layoutWidget.setObjectName("layoutWidget")
        self.verticalLayout = QtWidgets.QVBoxLayout(self.layoutWidget)
        self.verticalLayout.setContentsMargins(0, 0, 0, 0)
        self.verticalLayout.setObjectName("verticalLayout")
        self.physMemoryGraph = PlotWidget(self.layoutWidget)
        self.physMemoryGraph.setAutoFillBackground(True)
        self.physMemoryGraph.setObjectName("physMemoryGraph")
        self.verticalLayout.addWidget(self.physMemoryGraph)
        self.heapMemoryGraph = PlotWidget(self.layoutWidget)
        self.heapMemoryGraph.setAutoFillBackground(True)
        self.heapMemoryGraph.setObjectName("heapMemoryGraph")
        self.verticalLayout.addWidget(self.heapMemoryGraph)
        self.tabWidget.addTab(self.tab_Memory, "")
        self.tab_Processes = QtWidgets.QWidget()
        self.tab_Processes.setObjectName("tab_Processes")
        self.tabWidget.addTab(self.tab_Processes, "")
        self.label_2 = QtWidgets.QLabel(self.groupBox_3)
        self.label_2.setGeometry(QtCore.QRect(10, 584, 131, 17))
        self.label_2.setObjectName("label_2")
        self.refreshTimeBox = QtWidgets.QDoubleSpinBox(self.groupBox_3)
        self.refreshTimeBox.setGeometry(QtCore.QRect(150, 580, 111, 26))
        self.refreshTimeBox.setDecimals(2)
        self.refreshTimeBox.setMinimum(0.01)
        self.refreshTimeBox.setMaximum(50.0)
        self.refreshTimeBox.setSingleStep(0.1)
        self.refreshTimeBox.setProperty("value", 2.0)
        self.refreshTimeBox.setObjectName("refreshTimeBox")
        mainWindow.setCentralWidget(self.centralwidget)

        self.retranslateUi(mainWindow)
        self.tabWidget.setCurrentIndex(2)
        QtCore.QMetaObject.connectSlotsByName(mainWindow)

    def retranslateUi(self, mainWindow):
        _translate = QtCore.QCoreApplication.translate
        mainWindow.setWindowTitle(_translate("mainWindow", "CactusOS Debugger"))
        self.groupBox.setTitle(_translate("mainWindow", "Serial Log:"))
        self.serialSendButton.setText(_translate("mainWindow", "Send"))
        self.groupBox_2.setTitle(_translate("mainWindow", "Serial Settings"))
        self.connectButton.setText(_translate("mainWindow", "Connect"))
        self.label.setText(_translate("mainWindow", "COM Port:"))
        self.groupBox_3.setTitle(_translate("mainWindow", "Stats:"))
        self.tabWidget.setTabText(self.tabWidget.indexOf(self.tab_General), _translate("mainWindow", "General"))
        self.tabWidget.setTabText(self.tabWidget.indexOf(self.tab_CPU), _translate("mainWindow", "CPU"))
        self.tabWidget.setTabText(self.tabWidget.indexOf(self.tab_Disk), _translate("mainWindow", "Disk"))
        self.tabWidget.setTabText(self.tabWidget.indexOf(self.tab_Memory), _translate("mainWindow", "Memory"))
        self.tabWidget.setTabText(self.tabWidget.indexOf(self.tab_Processes), _translate("mainWindow", "Processes"))
        self.label_2.setText(_translate("mainWindow", "Update frequency:"))
from pyqtgraph import PlotWidget
