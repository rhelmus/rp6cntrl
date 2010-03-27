#include <QtCore>

#include "server.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    new CControl(&app);
    return app.exec();
}
