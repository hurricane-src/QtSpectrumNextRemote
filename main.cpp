#include <QApplication>
#include <QPushButton>
#include "SpectrumNextRemoteForm.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("Hurricane");
    QCoreApplication::setOrganizationDomain("hurrikhan.eu");
    QCoreApplication::setApplicationName("SpectrumNextRemote");
    QCoreApplication::setApplicationVersion("1.1.0");

    SpectrumNextRemoteForm spectrumNextRemoteForm;
    spectrumNextRemoteForm.show();
    return QApplication::exec();
}
