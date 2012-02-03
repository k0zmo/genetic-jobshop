#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include "Plot.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void changeEvent(QEvent *e);

private slots:
    void aboutActionHandler();
    void readProblemActionHandler();
    void quitActionHandler();
    void startAlgorithmActionHandler();

    void populationModelChanged(int idx);
    void selectionMethodChanged(int idx);

private:
    void connectSignalSlot();
    void setDefaultValues();
    void setValidators();
    void setProblemViewData(const QString &data);

    Ui::MainWindow *ui;
    QMenuBar *menuBar;
    QStatusBar *statusBar;

    QMenu *fileMenu;

    QAction *readProblemAction;
    QAction *saveReportAction;
    QAction *quitAction;
    QAction *aboutAction;
    QAction *startAlgorithmAction;

	Plot* d_plot;
	Zoomer* d_zoomer[2];
	QwtPlotPanner* d_panner;

    QTextBrowser *reportBrowser;
	QString fName;
	bool loaded;
};

#endif // MAINWINDOW_H
