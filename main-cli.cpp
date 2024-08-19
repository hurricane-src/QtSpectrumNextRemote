#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <iostream>
#include <QTimer>
#include "QtSpectrumNextRemoteTask.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCoreApplication::setOrganizationName("Hurricane");
    QCoreApplication::setOrganizationDomain("hurrikhan.eu");
    QCoreApplication::setApplicationName("SpectrumNextRemoteCLI");
    QCoreApplication::setApplicationVersion("1.1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Spectrum Next Remote");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption fileOption(QStringList() << "f" << "file", "The .nex executable (mandatory)", "file");
    parser.addOption(fileOption);
    QCommandLineOption hostOption(QStringList() << "H" << "host", "The address of the Spectrum Next (mandatory)", "address");
    parser.addOption(hostOption);
    QCommandLineOption rateOption(QStringList() << "r" << "rate", "The send rate in bytes per second", "rate");
    parser.addOption(rateOption);
    parser.process(app);

    int rate = 8192;
    bool options_ok = true;

    if(!parser.isSet(fileOption))
    {
        std::cerr << "mandatory file option not set" << std::endl;
        options_ok = false;
    }

    if(!parser.isSet(hostOption))
    {
        std::cerr << "mandatory host option not set" << std::endl;
        options_ok = false;
    }

    if(parser.isSet(rateOption))
    {
        rate = parser.value(rateOption).toInt();
        if(rate < 512)
        {
            rate = 512;
        }
        else if(rate > 8192)
        {
            rate = 8192;
        }
        std::cout << "rate set to " << rate << std::endl;
    }

    if(!options_ok)
    {
        std::cerr << "options not properly set: giving up" << std::endl;
        return EXIT_FAILURE;
    }

    QtSpectrumNextRemoteTask task(&app, parser.value(hostOption), parser.value(fileOption), rate);
    QObject::connect(&task, SIGNAL(finished()), &app, SLOT(quit()));
    QTimer::singleShot(0, &task, SLOT(run()));

    return QCoreApplication::exec();
}
