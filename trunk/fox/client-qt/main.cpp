#include <QApplication>

#include "client.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    (new CQtClient)->show();
    return app.exec();
}
