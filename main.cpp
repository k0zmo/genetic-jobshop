#include <QtGui/QApplication>
#include "MainWindow.h"

#if !defined(_WIN32)
#include <locale.h>
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

#if !defined(_WIN32)
	setlocale(LC_NUMERIC, "en_US.utf8");
#endif

	MainWindow w;
    w.show();
    return a.exec();
}
