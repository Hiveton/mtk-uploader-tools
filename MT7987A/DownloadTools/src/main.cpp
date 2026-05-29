#include "MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("Hiveton MTK Downloader Tools");
    QApplication::setOrganizationName("Hiveton");

    const QStringList args = QApplication::arguments();
    const bool smokeTest = args.contains("--smoke-test") || qEnvironmentVariableIsSet("DOWNLOADTOOLS_SMOKE_TEST");

    MainWindow window;
    if (smokeTest) {
        return 0;
    }
    window.show();
    return app.exec();
}
