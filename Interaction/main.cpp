#include <QApplication>
#include "principal.h"

int main( int argc, char **argv )
{
    QApplication app( argc, argv );

    Principal interaction;
    interaction.show();

    return app.exec();
}
