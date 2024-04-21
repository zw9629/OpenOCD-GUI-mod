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
	
#include <QApplication>
#include <QTranslator>
#include "mainwidget.h"

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QTranslator *translator=new QTranslator();
    bool bResult= translator->load(":/language/lang_zh_CN.qm");
    bResult=application.installTranslator(translator);
     application.setApplicationDisplayName("OpenOCD GUI");
	application.setApplicationName("openocd-gui");
	application.setOrganizationName("openocd-gui");
    application.setApplicationVersion("0.4.1");

    MainWidget widget;


    widget.show();
    return application.exec();
}
