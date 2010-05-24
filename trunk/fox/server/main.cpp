#include <QtCore>

#include "server.h"

int main(int argc, char **argv)
{
    QCoreApplication::setOrganizationName("rp6cntrl");
    QCoreApplication::setOrganizationDomain("code.google.com/p/rp6cntrl");
    QCoreApplication::setApplicationName("FoxServer");
    
    QCoreApplication app(argc, argv);
    new CControl(&app);
    return app.exec();
}
