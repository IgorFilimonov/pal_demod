#include <QApplication>
#include "view/atvview.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("PAL Demod");

    ATVView window;
    window.show();

    return app.exec();
}
