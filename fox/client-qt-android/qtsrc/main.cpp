#include <QApplication>

#include "client.h"

int main(int argc, char **argv)
{    
    QCoreApplication::setOrganizationName("rp6cntrl");
    QCoreApplication::setOrganizationDomain("code.google.com/p/rp6cntrl");
    QCoreApplication::setApplicationName("FoxQtClient");

    QApplication app(argc, argv);
    app.setAutoSipEnabled(true);
    (new CQtClient)->show();
    //(new CQtClient)->showFullScreen();
    return app.exec();
}
