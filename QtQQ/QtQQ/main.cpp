//#include "CCMainWindow.h"
#include "userlogin.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    /*CCMainWindow window;
    window.show();*/
    app.setQuitOnLastWindowClosed(false);

    UserLogin* userLogin = new UserLogin;
    userLogin->show();
    return app.exec();
}
