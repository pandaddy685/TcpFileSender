#include "tcpfilesender.h"
#include "tcpfileserver.h"
#include <QApplication>
#include"QTabWidget"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    TcpFileSender *w=new TcpFileSender;
    TcpFileServer *w1=new TcpFileServer;
    QTabWidget *s = new QTabWidget;
    s->addTab(w, "File Sender");
    s->addTab(w1, "File Server");
    s->show();

    return a.exec();
}
