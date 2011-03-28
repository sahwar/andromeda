#include <QtGui/QApplication>
#include <QDebug>

#include <GuiSystem>

#include "testview.h"
#include "testview2.h"

using namespace GuiSystem;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    GuiController::instance()->addFactory(new TestViewFactory);
    GuiController::instance()->addFactory(new TestViewFactory2);
    Perspective *perspective = new Perspective("Test Perspective");

    perspective->addView("TestViewFactory2", MainWindow::LeftViewArea);
    perspective->addView("TestViewFactory", MainWindow::CentralViewArea);

    GuiController::instance()->addPerspective(perspective);

    MainWindow w;
    w.setPerspective("Test Perspective");
    w.show();

    return app.exec();
}
