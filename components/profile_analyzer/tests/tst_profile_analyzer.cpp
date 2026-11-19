#include <QString>
#include <QTest>
#include <cmath>
#include <string>

#include "profile_analyzer/profile_analyzer.h"

namespace {
    constexpr double kPi = 3.14159265358979323846;

    struct CallResult {
        int code = 0;
        QString type;
        double height = 0.0;
        double weight = 0.0;
    };

    inline CallResult call(const QString& name) {
        const std::wstring w = name.toStdWString();
        wchar_t type[PA_PROFILE_TYPE_BUFFER_SIZE];
        std::fill(std::begin(type), std::end(type), L'#');
        CallResult r;
        r.code = AnalyzeProfile(w.c_str(), &r.height, &r.weight, type);
        r.type = QString::fromWCharArray(type);
        return r;
    }

    inline double steel(double areaMm2) {
        return areaMm2 * 7.85e-3;
    }
}  // namespace

class TestProfileAnalyzer : public QObject {
    Q_OBJECT

private slots:
    void analyze_data();
    void analyze();
    void nullArgumentsAreRejected();
    void tooLongNameIsRejected();
    void outputsAreResetOnError();
    void errorTextIsNeverNull();
};

void TestProfileAnalyzer::analyze_data() {
    QTest::addColumn<QString>("name");
    QTest::addColumn<int>("code");
    QTest::addColumn<QString>("type");
    QTest::addColumn<double>("height");
    QTest::addColumn<double>("weight");

    // Reference table
    QTest::newRow("IPE200") << "IPE200" << PA_OK << "IPE" << 200.0 << 22.4;
    QTest::newRow("HEA160") << "HEA160" << PA_OK << "HEA" << 160.0 << 30.4;
    QTest::newRow("HEB300") << "HEB300" << PA_OK << "HEB" << 300.0 << 117.0;
    QTest::newRow("UPN200") << "UPN200" << PA_OK << "UPN" << 200.0 << 25.3;
    QTest::newRow("lowercase+spaces") << "  ipe 200 " << PA_OK << "IPE" << 200.0 << 22.4;
    QTest::newRow("dash separator") << "IPE-200" << PA_OK << "IPE" << 200.0 << 22.4;
    QTest::newRow("interpolated") << "IPE210" << PA_OK << "IPE" << 210.0 << 24.3;

    // Geometry formulas
    QTest::newRow("L150*5") << "L150*5" << PA_OK << "L" << 150.0
                            << steel(5.0 * (300.0 - 5.0));
    QTest::newRow("L150x5 (×)")
        << QString::fromUtf8("L150×5") << PA_OK << "L" << 150.0 << steel(5.0 * 295.0);
    QTest::newRow("L100x75x8") << "L100x75x8" << PA_OK << "L" << 100.0
                               << steel(8.0 * 167.0);
    QTest::newRow("RHS100x50x4")
        << "RHS100x50x4" << PA_OK << "RHS" << 100.0 << steel(2.0 * 4.0 * 142.0);
    QTest::newRow("SHS80x4") << "SHS80x4" << PA_OK << "SHS" << 80.0
                             << steel(2.0 * 4.0 * 152.0);
    QTest::newRow("CHS114.3x4")
        << "CHS114.3x4" << PA_OK << "CHS" << 114.3 << steel(kPi * 4.0 * 110.3);
    QTest::newRow("CHS decimal comma")
        << "CHS114,3x4" << PA_OK << "CHS" << 114.3 << steel(kPi * 4.0 * 110.3);
    QTest::newRow("FL100x10") << "FL100x10" << PA_OK << "FL" << 100.0 << steel(1000.0);
    QTest::newRow("RD20") << "RD20" << PA_OK << "RD" << 20.0 << steel(kPi * 100.0);

    // Errors
    QTest::newRow("unknown type")
        << "XYZ100" << PA_E_UNKNOWN_TYPE << "XYZ" << 100.0 << 0.0;
    QTest::newRow("empty") << "" << PA_E_EMPTY_NAME << "" << 0.0 << 0.0;
    QTest::newRow("blank") << "   " << PA_E_EMPTY_NAME << "" << 0.0 << 0.0;
    QTest::newRow("no type") << "200" << PA_E_INVALID_FORMAT << "" << 0.0 << 0.0;
    QTest::newRow("no dimensions") << "IPE" << PA_E_INVALID_FORMAT << "" << 0.0 << 0.0;
    QTest::newRow("trailing garbage")
        << "IPE200 S235" << PA_E_INVALID_FORMAT << "" << 0.0 << 0.0;
    QTest::newRow("dangling separator")
        << "IPE200x" << PA_E_INVALID_FORMAT << "" << 0.0 << 0.0;
    QTest::newRow("too many dims for table")
        << "IPE200x100" << PA_E_INVALID_FORMAT << "IPE" << 200.0 << 0.0;
    QTest::newRow("angle without thickness")
        << "L150" << PA_E_INVALID_FORMAT << "L" << 150.0 << 0.0;
    QTest::newRow("above table")
        << "IPE1000" << PA_E_OUT_OF_RANGE << "IPE" << 1000.0 << 0.0;
    QTest::newRow("below table") << "IPE50" << PA_E_OUT_OF_RANGE << "IPE" << 50.0 << 0.0;
    QTest::newRow("thickness too big")
        << "L150*150" << PA_E_OUT_OF_RANGE << "L" << 150.0 << 0.0;
    QTest::newRow("zero dimension") << "L0*5" << PA_E_OUT_OF_RANGE << "L" << 0.0 << 0.0;
    QTest::newRow("type too long")
        << QString(40, u'A') + "100" << PA_E_INVALID_FORMAT << "" << 0.0 << 0.0;
}

void TestProfileAnalyzer::analyze() {
    QFETCH(QString, name);
    QFETCH(int, code);
    QFETCH(QString, type);
    QFETCH(double, height);
    QFETCH(double, weight);

    const CallResult r = call(name);
    QCOMPARE(r.code, code);
    QCOMPARE(r.type, type);
    QVERIFY2(std::abs(r.height - height) < 1e-9, qPrintable(QString::number(r.height)));
    QVERIFY2(std::abs(r.weight - weight) < 1e-6,
             qPrintable(QString::number(r.weight, 'f', 6)));
}

void TestProfileAnalyzer::nullArgumentsAreRejected() {
    double h = 1, w = 1;
    wchar_t type[PA_PROFILE_TYPE_BUFFER_SIZE] = L"junk";
    QCOMPARE(AnalyzeProfile(nullptr, &h, &w, type), PA_E_INVALID_ARGUMENT);
    QCOMPARE(h, 0.0);
    QCOMPARE(type[0], L'\0');
    QCOMPARE(AnalyzeProfile(L"IPE200", nullptr, &w, type), PA_E_INVALID_ARGUMENT);
    QCOMPARE(AnalyzeProfile(L"IPE200", &h, nullptr, type), PA_E_INVALID_ARGUMENT);
    QCOMPARE(AnalyzeProfile(L"IPE200", &h, &w, nullptr), PA_E_INVALID_ARGUMENT);
}

void TestProfileAnalyzer::tooLongNameIsRejected() {
    const QString name = "IPE" + QString(PA_PROFILE_NAME_MAX_LENGTH, u'0') + "1";
    QCOMPARE(call(name).code, PA_E_INVALID_FORMAT);
}

void TestProfileAnalyzer::outputsAreResetOnError() {
    double h = 123, w = 456;
    wchar_t type[PA_PROFILE_TYPE_BUFFER_SIZE] = L"OLD";
    QCOMPARE(AnalyzeProfile(L"???", &h, &w, type), PA_E_INVALID_FORMAT);
    QCOMPARE(h, 0.0);
    QCOMPARE(w, 0.0);
    QCOMPARE(type[0], L'\0');
}

void TestProfileAnalyzer::errorTextIsNeverNull() {
    for (int code : {PA_OK, PA_E_INVALID_ARGUMENT, PA_E_EMPTY_NAME, PA_E_INVALID_FORMAT,
                     PA_E_UNKNOWN_TYPE, PA_E_OUT_OF_RANGE, PA_E_INTERNAL, 12345}) {
        const wchar_t* text = GetAnalyzeProfileErrorText(code);
        QVERIFY(text != nullptr);
        QVERIFY(text[0] != L'\0');
    }
}

QTEST_APPLESS_MAIN(TestProfileAnalyzer)
#include "tst_profile_analyzer.moc"
