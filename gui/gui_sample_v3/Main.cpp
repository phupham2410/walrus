#include "MainWindow.h"
#include <QApplication>

#include "windows.h"
#include "shlobj.h"

int main(int argc, char *argv[])
{
    if (!IsUserAnAdmin()) {
        MessageBoxA(NULL,
            "Please run program in admin mode",
                "Corsair SSD ToolBox", MB_OK);
        return EXIT_FAILURE;
    }

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
