/*
Graphical frontend for the Open On-Chip Debugger
Copyright (C) 2013 Sven Sperner
Copyright (C) 2020 Karl Zeilhofer

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mainwidget.h"
#include "ui_mainwidget.h"
#include <QStringList>
#include <QScrollBar>
#include <QFileDialog>
#include <QByteArray>
#include <QRect>

#include <QSettings>
#include <iostream>
#include <qmessagebox.h>
#include <QTranslator>
#include <QDebug>
using namespace std;

MainWidget::MainWidget(QWidget *parent) : QWidget(parent), main(new Ui::MainWidget)
{
    main->setupUi(this);

    setGeometry(QRect(100,100,800,480));
	
	// set version strin in about tab:
	main->textEditHelp->setHtml(
				main->textEditHelp->toHtml().
				replace("__VERSION__", QCoreApplication::applicationVersion()));
	main->textEditHelp->setTextInteractionFlags(Qt::LinksAccessibleByMouse);

    openOCD = new QProcess(this);
    telnet = new QtTelnet(this);


	writeDefaultSettings();
	
	QSettings set;
	if(set.value("first-run", true).toBool()){
		set.setValue("first-run", false);

		QMessageBox::information(this, "Firt run of OpenOCD GUI", 
			 "This is your first execution of OpenOCD GUI. \n"
			 "Default values are used. \n"
			 "You can configure some of them in '~/.config/openocd-gui/openocd-gui.conf'.\n"
			 "To return to defaults, delete the line with the parameter.\n"
			 "If you do so, please close openocd-gui before that.");
	}

// load settings into line edits:
	main->tabWidget->setCurrentIndex(set.value("tabWidget", 0).toInt());
	
	main->lineEditOcdInterfaceConfig->setText(set.value("lineEditOcdInterfaceConfig").toString());
	main->lineEditOcdTargetConfig->setText(set.value("lineEditOcdTargetConfig").toString());
	
	main->lineEditRam->setText(set.value("lineEditRam").toString());
	main->lineEditFlash->setText(set.value("lineEditFlash").toString());
	
	main->lineEditHost->setText(set.value("lineEditHost").toString());
	main->lineEditPort->setText(set.value("lineEditPort").toString());
	main->lineEditFlash->setText(set.value("lineEditFlash").toString());
	main->checkBoxErase->setChecked(set.value("checkBoxErase").toBool());
	main->lineEditRam->setText(set.value("lineEditRam").toString());
	
	main->lineEditGuiConfig->setText(set.value("lineEditGuiConfig").toString());

// control buttons
    connect(main->pushButtonOocdConnect, SIGNAL(clicked()), this, SLOT(connectToServer()));
    connect(telnet, SIGNAL(message(QString)), this, SLOT(telnetMessage(QString)));
    connect(telnet->socket(), SIGNAL(connected()), this, SLOT(telnetConnected()));
    connect(telnet, SIGNAL(connectionError(QAbstractSocket::SocketError)), this, SLOT(telnetConnectionError()));

    connect(main->pushButtonOocdReset, SIGNAL(clicked()), this, SLOT(resetOocd()));
    connect(main->lineEditInput, SIGNAL(returnPressed()), this, SLOT(telnetData()));

// ram
    connect(main->pushButtonRamFile, SIGNAL(clicked()), this, SLOT(ramFileSelect()));
    connect(main->pushButtonRamLoad, SIGNAL(clicked()), this, SLOT(ramLoad()));

// flash
    connect(main->pushButtonFlashFile, SIGNAL(clicked()), this, SLOT(flashFileSelect()));
    connect(main->pushButtonFlashLoad, SIGNAL(clicked()), this, SLOT(flashLoad()));

// command buttons
    connect(main->pushButtonSoftReset, SIGNAL(clicked()), this, SLOT(softReset()));
    connect(main->pushButtonReset, SIGNAL(clicked()), this, SLOT(reset()));
    connect(main->pushButtonResume, SIGNAL(clicked()), this, SLOT(resume()));
    connect(main->pushButtonHalt, SIGNAL(clicked()), this, SLOT(halt()));
    connect(main->pushButtonPoll, SIGNAL(clicked()), this, SLOT(poll()));
    connect(main->pushButtonShowMem, SIGNAL(clicked()), this, SLOT(showMemory()));
    connect(main->pushButtonEraseFlash, SIGNAL(clicked()), this, SLOT(eraseFlash()));
    connect(main->pushButtonRemap, SIGNAL(clicked()), this, SLOT(remap()));
    connect(main->pushButtonPeriphReset, SIGNAL(clicked()), this, SLOT(peripheralReset()));
    connect(main->pushButtonCpuReset, SIGNAL(clicked()), this, SLOT(cpuReset()));

// openocd tab
	connect(main->pushButtonOcdInterfaceConfigFile, SIGNAL(clicked()), this, SLOT(ocdInterfaceConfigFileSelect()));
	connect(main->pushButtonOcdTargetConfigFile, SIGNAL(clicked()), this, SLOT(ocdTargetConfigFileSelect()));
    connect(main->pushButtonOcdDaemonStart, SIGNAL(clicked()), this, SLOT(ocdDaemonStart()));

    connect(openOCD, SIGNAL(readyRead()), this, SLOT(openOcdMessage()));
    connect(openOCD, SIGNAL(readyReadStandardOutput()), this, SLOT(openOcdStdout()));
    connect(openOCD, SIGNAL(readyReadStandardError()), this, SLOT(openOcdStderr()));

// configuration tab
    connect(main->pushButtonGuiConfigFile, SIGNAL(clicked()), this, SLOT(selectConfigFile()));
    connect(main->pushButtonGuiConfigLoad, SIGNAL(clicked()), this, SLOT(loadConfiguration()));
    connect(main->pushButtonGuiConfigSave, SIGNAL(clicked()), this, SLOT(saveConfiguration()));
}

MainWidget::~MainWidget()
{
    delete main;
}



// private Slots:

// control buttons:
void MainWidget::connectToServer()
{
    if (main->pushButtonOocdConnect->text() == tr("Connect"))
    {
        telnet->connectToHost(main->lineEditHost->text(), quint16(main->lineEditPort->text().toInt()));
    }
    else
    {
        telnet->close();
        main->textEditOutput->append("GUI: Connection closed");
        main->pushButtonOocdConnect->setText(tr("Connect"));
    }
}

void MainWidget::telnetConnected()
{
    main->textEditOutput->append("GUI: Connected to " + main->lineEditHost->text() + ":" + main->lineEditPort->text());
    main->pushButtonOocdConnect->setText(tr("Disconnect"));
}

void MainWidget::telnetConnectionError()
{
    main->textEditOutput->append("GUI: Can not connect to " + main->lineEditHost->text() + ":" + main->lineEditPort->text());
}

void MainWidget::resetOocd()
{
    if (main->pushButtonOocdConnect->text() == tr("Disconnect"))
    {
        telnet->close();
        telnet->connectToHost(main->lineEditHost->text(), quint16(main->lineEditPort->text().toInt()));
        main->textEditOutput->append("GUI: Reset Connection");
    }
    else
    {
        main->textEditOutput->append("GUI: Not connected");
    }
}


void MainWidget::telnetMessage(const QString &msg) // receive output
{
    main->textEditOutput->append(stripCR(msg));
    QScrollBar *s = main->textEditOutput->verticalScrollBar();
    s->setValue(s->maximum());
}

void MainWidget::telnetData() // send command
{
    telnet->sendData(main->lineEditInput->text());
    main->lineEditInput->clear();
}


void MainWidget::ramFileSelect()
{
	QSettings set;
    QFileDialog fDlg(this, "Select RAM Image", 
					 set.value("lineEditRam").toString(), 
                     "*.hex *.HEX *.bin *.BIN *.elf *.ELF");

    if(fDlg.exec())
    {
        main->lineEditRam->setText(fDlg.selectedFiles().at(0));
    }
}

void MainWidget::ramLoad() // download image to RAM
{
    QString buffer = main->lineEditRam->text();
    int tmp = buffer.size();
    
    if ((buffer[tmp-3] == 'e' && buffer[tmp-2] == 'l' && buffer[tmp-1] == 'f') ||
        (buffer[tmp-3] == 'E' && buffer[tmp-2] == 'L' && buffer[tmp-1] == 'F'))
    {
        telnet->sendData(main->lineEditSoftResetCmd->text());
        telnet->sendData("load_image " + main->lineEditRam->text() + " " + 
						 main->lineEditBaseAddress->text() + " " +
						 "elf");
    }
    else if ((buffer[tmp-3] == 'b' && buffer[tmp-2] == 'i' && buffer[tmp-1] == 'n') ||
	     (buffer[tmp-3] == 'B' && buffer[tmp-2] == 'I' && buffer[tmp-1] == 'N'))
    {
        telnet->sendData(main->lineEditSoftResetCmd->text());
        telnet->sendData("load_image " + main->lineEditRam->text() + " " + 
						 main->lineEditRamAddress->text() + " " +
						 "bin");
    }
}

void MainWidget::flashFileSelect()
{
	QSettings set;
    QFileDialog fDlg(this, "Select FLASH Image", 
					 set.value("lineEditFlash").toString(),
                     "*.hex *.HEX *.bin *.BIN *.elf *.ELF");

    if(fDlg.exec())
    {
        main->lineEditFlash->setText(fDlg.selectedFiles().at(0));
    }
}

// TODO Test Flash Write for ELF and BIN
void MainWidget::flashLoad() // download image to FLASH
{
	QString buffer = main->lineEditFlash->text();
	int tmp = buffer.size();
	
	if ((buffer[tmp-3] == 'e' && buffer[tmp-2] == 'l' && buffer[tmp-1] == 'f') ||
			(buffer[tmp-3] == 'E' && buffer[tmp-2] == 'L' && buffer[tmp-1] == 'F'))
	{
		telnet->sendData(main->lineEditSoftResetCmd->text());
		if (main->checkBoxErase->isChecked())
			telnet->sendData(main->lineEditFlashWriteCmd->text() + " " + 
							 main->lineEditEraseSuffix->text() + " " + 
							 main->lineEditFlash->text() + " " + 
							 main->lineEditBaseAddress->text() + " " + 
							 "elf");
		else
			telnet->sendData(main->lineEditFlashWriteCmd->text() + " " + 
							 main->lineEditFlash->text() + " " + 
							 main->lineEditBaseAddress->text() + " " + 
							 "elf");
	}
	else if ((buffer[tmp-3] == 'b' && buffer[tmp-2] == 'i' && buffer[tmp-1] == 'n') ||
			 (buffer[tmp-3] == 'B' && buffer[tmp-2] == 'I' && buffer[tmp-1] == 'N'))
	{
		telnet->sendData(main->lineEditSoftResetCmd->text());
		if (main->checkBoxErase->isChecked())
		{
			telnet->sendData(main->lineEditFlashWriteCmd->text() + " " + 
							 main->lineEditEraseSuffix->text() + " " + 
							 main->lineEditFlash->text() + " " + 
							 main->lineEditFlashAddress->text() + " " + 
							 "bin");
		}
		else
		{
			telnet->sendData(main->lineEditFlashWriteCmd->text() + " " + 
							 main->lineEditFlash->text() + " " +
							 main->lineEditFlashAddress->text() + " " + 
							 "bin");
		}
	}
    else if((buffer[tmp-3] == 'h' && buffer[tmp-2] == 'e' && buffer[tmp-1] == 'x') ||
             (buffer[tmp-3] == 'H' && buffer[tmp-2] == 'E' && buffer[tmp-1] == 'X'))
        {
            if(main->checkBoxErase->isChecked())
            {
            telnet->sendData(main->lineEditFlashWriteCmd->text() + " " +
                            main->lineEditEraseSuffix->text() + " " +
                            main->lineEditFlash->text() + ";" );
            }
            else
            {
                telnet->sendData(main->lineEditFlashWriteCmd->text() + " " +
                     main->lineEditFlash->text() + ";" );
            }
        }
}



// command buttons:
void MainWidget::softReset()
{
	// soft_reset_halt
    telnet->sendData(main->lineEditSoftResetCmd->text());
}

void MainWidget::reset()
{
    telnet->sendData(main->lineEditResetCmd->text());
}

void MainWidget::halt()
{
    telnet->sendData(main->lineEditHaltCmd->text());
}

void MainWidget::resume()
{
    telnet->sendData(main->lineEditResumeCmd->text());
}

void MainWidget::poll()
{
    telnet->sendData(main->lineEditPollCmd->text());
}

void MainWidget::eraseFlash()
{
    telnet->sendData(main->lineEditSoftResetCmd->text());
    telnet->sendData(main->lineEditFlashEraseCmd->text());
}

//
void MainWidget::showMemory()
{
    telnet->sendData("mdw " + main->lineEditBaseAddress->text() + " 0x08");	// base (mapped)
    telnet->sendData("mdw " + main->lineEditFlashAddress->text() + " 0x08");	// flash
    telnet->sendData("mdw " + main->lineEditRamAddress->text() + " 0x08");	// sram
}

void MainWidget::remap()
{
    telnet->sendData("mww " + main->lineEditRemapAddress->text() + " " + main->lineEditRemapValue->text());
}

void MainWidget::peripheralReset()
{
    telnet->sendData("mww " + main->lineEditPeriphResetAddress->text() + " " + main->lineEditPeriphResetValue->text());
}

void MainWidget::cpuReset()
{
    telnet->sendData("mww " + main->lineEditCpuResetAddress->text() + " " + main->lineEditCpuResetValue->text());
}



// openocd tab
void MainWidget::ocdInterfaceConfigFileSelect()
{
	QSettings set;
    qDebug()<< "./openocd/openocd/scripts/interface/";
    QFileDialog fDlg(this, "Select OpenOCD Interface Configuration", 
                     //set.value("lineEditOcdInterfaceConfig").toString(),
                     qApp->applicationDirPath()+"/openocd/openocd/scripts/interface/",
					 "*.cfg");

    if(fDlg.exec())
    {
        main->lineEditOcdInterfaceConfig->setText(fDlg.selectedFiles().at(0));
    }
}

// openocd tab
void MainWidget::ocdTargetConfigFileSelect()
{
	QSettings set;
    QFileDialog fDlg(this, "Select OpenOCD Target Configuration", 
                     qApp->applicationDirPath()+"/openocd/openocd/scripts/target/",
					 "*.cfg");

    if(fDlg.exec())
    {
        main->lineEditOcdTargetConfig->setText(fDlg.selectedFiles().at(0));
    }
}

void MainWidget::ocdDaemonStart() // start OpenOCD with Config
{
	QSettings set;

    if (main->pushButtonOcdDaemonStart->text() == tr("Start"))
    {
        QString path = set.value("openocd-bin-path").toString();
		
        QStringList arguments;
		QString interfaceConfig = main->lineEditOcdInterfaceConfig->text();
		QString targetConfig = main->lineEditOcdTargetConfig->text();
        arguments << "-f" << interfaceConfig << "-f" << targetConfig;
        openOCD->setWorkingDirectory(QCoreApplication::applicationDirPath());
        openOCD->start(QCoreApplication::applicationDirPath()+"/openocd/bin/openocd.exe ", arguments);

        qDebug()<<QCoreApplication::applicationDirPath()+"/openocd/bin/openocd.exe "+arguments.join(" ");
        QString outputString(openOCD->readAllStandardOutput());
        main->textEditOcdTerminal->append(outputString);
        main->textEditOcdTerminal->append(tr("GUI: Starting OpenOCD daemon..."));
		main->textEditOcdTerminal->append(path + arguments.join(" "));
        main->pushButtonOcdDaemonStart->setText(tr("Stop"));
    }
    else
    {
        openOCD->terminate();
        if (!openOCD->waitForFinished(1000))
            openOCD->kill();
        main->textEditOcdTerminal->append(tr("GUI: OpenOCD daemon stopped"));
        main->pushButtonOcdDaemonStart->setText(tr("Start"));
    }
}

void MainWidget::openOcdMessage()
{
    main->textEditOcdTerminal->append(stripCR(openOCD->readAll()));
    QScrollBar *s = main->textEditOcdTerminal->verticalScrollBar();
    s->setValue(s->maximum());
}

void MainWidget::openOcdStdout()
{
    main->textEditOcdTerminal->append(stripCR(openOCD->readAllStandardOutput()));
    QScrollBar *s = main->textEditOcdTerminal->verticalScrollBar();
    s->setValue(s->maximum());
}

void MainWidget::openOcdStderr()
{
    main->textEditOcdTerminal->append(stripCR(openOCD->readAllStandardError()));
    QScrollBar *s = main->textEditOcdTerminal->verticalScrollBar();
    s->setValue(s->maximum());
}

// configuration tab
void MainWidget::selectConfigFile()
{
	QSettings set;
    QFileDialog fDlg(this, "Select GUI Configuration", 
					 set.value("lineEditGuiConfig").toString()
					 , "*.openocd-gui.conf");

    if(fDlg.exec())
    {
        main->lineEditGuiConfig->setText(fDlg.selectedFiles().at(0));
    }
}

void MainWidget::loadConfiguration()
{
    QString buffer;
    QStringList buflist;
    QFile cfgFile(main->lineEditGuiConfig->text());
    if (cfgFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream cfgIn(&cfgFile);
        while (!cfgIn.atEnd())
        {
            buffer = cfgIn.readLine();
            if (buffer[0] == '#')	continue;	//skip comments
            buflist = buffer.split(' ');
            if (buflist[0] == "BASE") {
                main->lineEditBaseAddress->setText(buflist[2]);
            }
            else if (buflist[0] == "FLASH") {
                main->lineEditFlashAddress->setText(buflist[2]);
            }
            else if (buflist[0] == "RAM") {
                main->lineEditRamAddress->setText(buflist[2]);
            }
            else if (buflist[0] == "REMAP") {
                main->lineEditRemapAddress->setText(buflist[2]);
                main->lineEditRemapValue->setText(buflist[3]);
            }
            else if (buflist[0] == "RESETCPU") {
                main->lineEditCpuResetAddress->setText(buflist[2]);
                main->lineEditCpuResetValue->setText(buflist[3]);
            }
            else if (buflist[0] == "RESETPERIPH") {
                main->lineEditPeriphResetAddress->setText(buflist[2]);
                main->lineEditPeriphResetValue->setText(buflist[3]);
            }
            else if (buflist[0] == "FLASHPROBE") {
                main->lineEditFlashProbeCmd->setText(buflist[2]+" "+buflist[3]+" "+buflist[4]);
            }
            else if (buflist[0] == "FLASHINFO") {
                main->lineEditFlashInfoCmd->setText(buflist[2]+" "+buflist[3]+" "+buflist[4]);
            }
            else if (buflist[0] == "FLASHERASE") {
                main->lineEditFlashEraseCmd->setText(buflist[2]+" "+buflist[3]+" "+buflist[4]+" "+buflist[5]);
            }
            else if (buflist[0] == "FLASHUNLOCK") {
                main->lineEditFlashUnlockCmd->setText(buflist[2]+" "+buflist[3]+" "+buflist[4]+" "+buflist[5]+" "+buflist[6]);
            }
			else if (buflist[0] == "FLASHWRITE") {
                main->lineEditFlashWriteCmd->setText(buflist[2]+" "+buflist[3]);
            }
			else if (buflist[0] == "VERIFYIMAGE") {
                main->lineEditVerifyImageCmd->setText(buflist[2]);
            }
            else if (buflist[0] == "ERASESUFFIX") {
                main->lineEditEraseSuffix->setText(buflist[2]);
            }
            else if (buflist[0] == "RAMWRITE") {
                main->lineEditRamWriteCmd->setText(buflist[2]);
            }
            else if (buflist[0] == "RESET") {
                main->lineEditResetCmd->setText(buflist[2]);
            }
            else if (buflist[0] == "HALT") {
                main->lineEditHaltCmd->setText(buflist[2]);
            }
            else if (buflist[0] == "RESUME") {
                main->lineEditResumeCmd->setText(buflist[2]);
            }
            else if (buflist[0] == "POLL") {
                main->lineEditPollCmd->setText(buflist[2]);
            }
            else if (buflist[0] == "SOFTRESET") {
                main->lineEditSoftResetCmd->setText(buflist[2]);
            }
        }
        main->textEditOcdTerminal->append("GUI: GUI-Config loaded");
    }
}

void MainWidget::saveConfiguration()
{
    QFile cfgFile(main->lineEditGuiConfig->text());
    if (cfgFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream cfgOut(&cfgFile);
        cfgOut << "BASE = " << main->lineEditBaseAddress->text() << " " << endl;
        cfgOut << "FLASH = " << main->lineEditFlashAddress->text() << " " << endl;
        cfgOut << "RAM = " << main->lineEditRamAddress->text() << " " << endl;
        cfgOut << "REMAP = " << main->lineEditRemapAddress->text() << " "
                     << main->lineEditRemapValue->text() << endl;
        cfgOut << "RESETCPU = " << main->lineEditCpuResetAddress->text() << " "
                    << main->lineEditCpuResetValue->text() << endl;
        cfgOut << "RESETPERIPH = " << main->lineEditPeriphResetAddress->text() << " "
                       << main->lineEditPeriphResetValue->text() << endl;
        cfgOut << "FLASHPROBE = " << main->lineEditFlashProbeCmd->text() << " " << endl;
        cfgOut << "FLASHINFO = " << main->lineEditFlashInfoCmd->text() << " " << endl;
        cfgOut << "FLASHERASE = " << main->lineEditFlashEraseCmd->text() << " " << endl;
        cfgOut << "FLASHUNLOCK = " << main->lineEditFlashUnlockCmd->text() << " " << endl;
        cfgOut << "FLASHWRITE = " << main->lineEditFlashWriteCmd->text() << " " << endl;
		cfgOut << "VERIFYIMAGE = " << main->lineEditVerifyImageCmd->text() << " " << endl;
        cfgOut << "ERASESUFFIX = " << main->lineEditEraseSuffix->text() << " " << endl;
        cfgOut << "RAMWRITE = " << main->lineEditRamWriteCmd->text() << " " << endl;
        cfgOut << "RESET = " << main->lineEditResetCmd->text() << " " << endl;
        cfgOut << "HALT = " << main->lineEditHaltCmd->text() << " " << endl;
        cfgOut << "RESUME = " << main->lineEditResumeCmd->text() << " " << endl;
        cfgOut << "POLL = " << main->lineEditPollCmd->text() << " " << endl;
		cfgOut << "SOFTRESET = " << main->lineEditSoftResetCmd->text() << " " << endl;
        main->textEditOcdTerminal->append("GUI: GUI-Config saved as " + cfgFile.fileName());
    }
}



// private Funktions:
QString MainWidget::stripCR(const QString &msg)
{
    QString nmsg(msg);
    nmsg.remove('\r');
    //nmsg.remove('\n\r\n\r');
    nmsg.remove(QRegExp("\033\\[[0-9;]*[A-Za-z]")); // Also remove terminal control codes
    return nmsg.isEmpty() ? nullptr : nmsg;
}

void MainWidget::removeEmptyLines()
{
	
}

void MainWidget::writeDefaultSettings()
{
	QSettings set;
	QStringList keys = set.allKeys();
	
	defaultSettings.clear();
	defaultSettings["first-run"] = "true";
    defaultSettings["openocd-bin-path"] = "./openocd/bin/openocd.exe ";
	
    defaultSettings["lineEditOcdInterfaceConfig"] = "./openocd/openocd/scripts/interface/cmsis-dap.cfg";
    defaultSettings["lineEditOcdTargetConfig"] = "./openocd/openocd/scripts/target/stm32g0x.cfg";
	
	defaultSettings["lineEditHost"] = "localhost";
	defaultSettings["lineEditPort"] = "4444";
    defaultSettings["checkBoxErase"] = "false";
    defaultSettings["lineEditGuiConfig"] ="./mcu.openocd-gui.conf";
	
	for(int i=0; i<defaultSettings.size(); i++){
		QString defKey = defaultSettings.keys().at(i);
		
		if(set.contains(defKey) == false){
			set.setValue(defKey, defaultSettings[defKey]);
		}
	}
}

void MainWidget::on_pushButtonOcdShellClear_clicked()
{
    main->textEditOcdTerminal->clear();
}

void MainWidget::on_lineEditOcdInterfaceConfig_textChanged(const QString &arg1)
{
	QSettings set;
	set.setValue("lineEditOcdInterfaceConfig", main->lineEditOcdInterfaceConfig->text());
}

void MainWidget::on_lineEditOcdTargetConfig_textChanged(const QString &arg1)
{
	QSettings set;
	set.setValue("lineEditOcdTargetConfig", main->lineEditOcdTargetConfig->text());
}

void MainWidget::on_lineEditHost_textChanged(const QString &arg1)
{
	QSettings set;
	set.setValue("lineEditHost", main->lineEditHost->text());
}

void MainWidget::on_lineEditPort_textChanged(const QString &arg1)
{
	QSettings set;
	set.setValue("lineEditPort", main->lineEditPort->text());
}

void MainWidget::on_lineEditFlash_textChanged(const QString &arg1)
{
    QSettings set;
	set.setValue("lineEditFlash", main->lineEditFlash->text());
}

void MainWidget::on_checkBoxErase_stateChanged(int arg1)
{
	QSettings set;
	set.setValue("checkBoxErase", main->checkBoxErase->isChecked());
}

void MainWidget::on_lineEditRam_textChanged(const QString &arg1)
{
	QSettings set;
	set.setValue("lineEditRam", main->lineEditRam->text());
}

void MainWidget::on_lineEditGuiConfig_textChanged(const QString &arg1)
{
	QSettings set;
	set.setValue("lineEditGuiConfig", main->lineEditGuiConfig->text());
}

void MainWidget::on_tabWidget_currentChanged(int index)
{
    QSettings set;
	set.setValue("tabWidget", index);
}

// TODO Test Flash Verify for ELF and BIN
void MainWidget::on_pushButtonFlashVerify_clicked()
{
	QString buffer = main->lineEditFlash->text();
	
    int tmp = buffer.size();
    
    if ((buffer[tmp-3] == 'e' && buffer[tmp-2] == 'l' && buffer[tmp-1] == 'f') ||
        (buffer[tmp-3] == 'E' && buffer[tmp-2] == 'L' && buffer[tmp-1] == 'F'))
    {
        telnet->sendData(main->lineEditSoftResetCmd->text());
        telnet->sendData(main->lineEditVerifyImageCmd->text() + " " + 
						 main->lineEditFlash->text() + " " + 
						 main->lineEditBaseAddress->text() + " elf");
    }
    else if ((buffer[tmp-3] == 'b' && buffer[tmp-2] == 'i' && buffer[tmp-1] == 'n') ||
	     (buffer[tmp-3] == 'B' && buffer[tmp-2] == 'I' && buffer[tmp-1] == 'N'))
    {
        telnet->sendData(main->lineEditSoftResetCmd->text());
        telnet->sendData(main->lineEditVerifyImageCmd->text() + " " + 
						 main->lineEditFlash->text() + " " + 
						 main->lineEditFlashAddress->text() + " bin");
    }
}


// TODO Test Ram Verify for ELF and BIN
void MainWidget::on_pushButtonRamVerify_clicked()
{
	QString buffer = main->lineEditRam->text();
	
    int tmp = buffer.size();
    
    if ((buffer[tmp-3] == 'e' && buffer[tmp-2] == 'l' && buffer[tmp-1] == 'f') ||
        (buffer[tmp-3] == 'E' && buffer[tmp-2] == 'L' && buffer[tmp-1] == 'F'))
    {
        telnet->sendData(main->lineEditSoftResetCmd->text());
        telnet->sendData(main->lineEditVerifyImageCmd->text() + " " + 
						 main->lineEditRam->text() + " " + 
						 main->lineEditBaseAddress->text() + " elf");
    }
    else if ((buffer[tmp-3] == 'b' && buffer[tmp-2] == 'i' && buffer[tmp-1] == 'n') ||
	     (buffer[tmp-3] == 'B' && buffer[tmp-2] == 'I' && buffer[tmp-1] == 'N'))
    {
        telnet->sendData(main->lineEditSoftResetCmd->text());
        telnet->sendData(main->lineEditVerifyImageCmd->text() + " " + 
						 main->lineEditRam->text() + " " + 
						 main->lineEditRamAddress->text() + " bin");
    }
}

void MainWidget::on_pushButtonSubmit_clicked()
{
    telnet->sendData(main->lineEditInput->text());
}

void MainWidget::on_pushButtonOcdInterfaceConfigFile_clicked()
{

}


void MainWidget::on_pushButtonOcdInterfaceConfigFile_clicked(bool checked)
{

}


void MainWidget::on_pushButtonOcdDaemonStart_clicked()
{

}

