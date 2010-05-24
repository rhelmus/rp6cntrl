#include <QApplication>

#include "client.h"

int main(int argc, char **argv)
{
    QCoreApplication::setOrganizationName("rp6cntrl");
    QCoreApplication::setOrganizationDomain("code.google.com/p/rp6cntrl");
    QCoreApplication::setApplicationName("FoxClient");
    
    QApplication app(argc, argv);
    (new CQtClient)->show();
    return app.exec();
}
