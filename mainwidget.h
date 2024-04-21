/*
Graphical frontend for the Open On-Chip Debugger
Copyright (C) 2013 Sven Sperner

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

#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QProcess>
//#include <QtTelnet>
#include "QtTelnet/qttelnet.h"
#include <QtWidgets/QWidget>
#include <QFile>
#include <QTranslator>

#define DIR_FILE_NAME "/tmp/oocdqt-recentdir.dat"
#define DEFAULT_INTERFACES "/usr/local/share/openocd/scripts/interface"
#define DEFAULT_TARGETS "/usr/local/share/openocd/scripts/target"
#define DEFAULT_OPENOCD "/usr/bin/openocd"


namespace Ui
{
    class MainWidget;
}

class MainWidget : public QWidget
{
    Q_OBJECT

public:

    MainWidget(QWidget *parent = 0);
    ~MainWidget();

private:
    QString stripCR(const QString &msg);
    void removeEmptyLines();
	void writeDefaultSettings();


private slots:
// control buttons:
    void connectToServer();
    void telnetConnected();
    void telnetConnectionError();
    void resetOocd();
    void telnetMessage(const QString &msg);
    void telnetData();
    void ramFileSelect();
    void ramLoad();
    void flashFileSelect();
    void flashLoad();
// command buttons:
    void softReset();
    void reset();
    void halt();
    void resume();
    void poll();
    void eraseFlash();
    void showMemory();
    void remap();
    void peripheralReset();
    void cpuReset();
// openocd tab:
	void ocdInterfaceConfigFileSelect();
	void ocdTargetConfigFileSelect();
    void ocdDaemonStart();
    void openOcdMessage();
    void openOcdStdout();
    void openOcdStderr();

// config tab
    void selectConfigFile();
    void loadConfiguration();
    void saveConfiguration();

	void on_lineEditOcdInterfaceConfig_textChanged(const QString &arg1);
	
	void on_lineEditOcdTargetConfig_textChanged(const QString &arg1);
	
	void on_pushButtonFlashVerify_clicked();
	
	void on_pushButtonOcdShellClear_clicked();
	
	void on_checkBoxErase_stateChanged(int arg1);
	
	void on_lineEditFlash_textChanged(const QString &arg1);
	
	void on_lineEditHost_textChanged(const QString &arg1);
	
	void on_lineEditPort_textChanged(const QString &arg1);
	
	void on_lineEditGuiConfig_textChanged(const QString &arg1);
	
	
	void on_lineEditRam_textChanged(const QString &arg1);
	
	void on_tabWidget_currentChanged(int index);
	
	void on_pushButtonRamVerify_clicked();
	
	void on_pushButtonSubmit_clicked();

    void on_pushButtonOcdInterfaceConfigFile_clicked();

    void on_pushButtonOcdInterfaceConfigFile_clicked(bool checked);

    void on_pushButtonOcdDaemonStart_clicked();

private:
    Ui::MainWidget *main;
    QProcess *openOCD;
    QtTelnet *telnet;
    QString recentDir;
	QMap<QString, QString> defaultSettings;
};

#endif // MAINWIDGET_H
