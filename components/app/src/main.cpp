#include <QApplication>
#include <QCommandLineParser>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("BOM Analyzer"));
    QApplication::setApplicationVersion(QStringLiteral(BOM_APP_VERSION));
    QApplication::setOrganizationName(QStringLiteral("BomAnalyzer"));

    QTranslator qtTranslator;
    if (qtTranslator.load(QLocale(), QStringLiteral("qtbase"), QStringLiteral("_"),
                          QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        QApplication::installTranslator(&qtTranslator);

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("BOM loading, visualization and analytics"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("file"),
                                 QStringLiteral("CSV file to open"));
    parser.process(app);

    bom::app::MainWindow window;
    window.show();

    if (!parser.positionalArguments().isEmpty())
        window.openFile(parser.positionalArguments().constFirst());

    return QApplication::exec();
}
