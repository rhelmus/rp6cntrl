#include <QtCore>

#include "client.h"

int main(int argc, char **argv)
{
    QCoreApplication::setOrganizationName("rp6cntrl");
    QCoreApplication::setOrganizationDomain("code.google.com/p/rp6cntrl");
    QCoreApplication::setApplicationName("FoxNCursClient");
    
    QCoreApplication app(argc, argv);
    new CNCursManager(&app);
    
    return app.exec();
} 
