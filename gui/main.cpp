#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "ChainModel.h"

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    ChainModel chainModel;
    chainModel.start();

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("chainModel", &chainModel);
    engine.loadFromModule("MultieffectGui", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
