#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>
#include <QDir>

#include "ChainModel.h"

int main(int argc, char* argv[])
{
    // "Basic" (domyślny styl QtQuick.Controls) tylko częściowo honoruje
    // palette — część kontrolek (np. tło Button) ma kolory zakodowane na
    // sztywno w samym stylu, niezależnie od palette.button itp. "Fusion"
    // w pełni idzie za palette (tak jak QWidgets), więc nasz motyw
    // dark/light w Main.qml faktycznie działa. Musi być ustawione PRZED
    // utworzeniem QGuiApplication.
    QQuickStyle::setStyle("Fusion");

    QGuiApplication app(argc, argv);

    // ikona okna (górny lewy róg) — plik obok .exe, kopiowany tam przez CMake
    // (ten sam mechanizm co assets/effects). Podmień gui/assets/icon.png na
    // własną, żeby zmienić — nie trzeba nic ruszać w kodzie.
    app.setWindowIcon(QIcon(QDir(app.applicationDirPath()).filePath("assets/icon.png")));

    ChainModel chainModel;
    chainModel.start();

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("chainModel", &chainModel);
    engine.loadFromModule("MultieffectGui", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
