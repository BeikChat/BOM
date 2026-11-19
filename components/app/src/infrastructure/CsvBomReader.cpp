#include "infrastructure/CsvBomReader.h"

#include <QCoreApplication>
#include <QFile>
#include <QHash>
#include <QLocale>
#include <QStringList>
#include <QTextStream>
#include <algorithm>
#include <cmath>

namespace bom::infrastructure {
    namespace {
        using core::BomItem;

        enum Column {
            ColBuildingId,
            ColProfileName,
            ColLengthMm,
            ColCost,
            ColQuantity,
            ColumnCount
        };

        inline const QStringList& expectedHeader() {
            static const QStringList header = {
                QStringLiteral("BuildingID"), QStringLiteral("ProfileName"),
                QStringLiteral("Length_mm"), QStringLiteral("Cost"),
                QStringLiteral("Quantity")};
            return header;
        }

        inline QString tr(const char* text) {
            return QCoreApplication::translate("CsvBomReader", text);
        }

        inline bool sameName(const QString& a, const QString& b) {
            return a.trimmed().compare(b, Qt::CaseInsensitive) == 0;
        }

        inline bool parseDouble(const QString& text, double& value) {
            bool ok = false;
            value = QLocale::c().toDouble(text, &ok);
            return ok && std::isfinite(value);
        }

        inline bool parseInt(const QString& text, int& value) {
            bool ok = false;
            value = QLocale::c().toInt(text, &ok);
            return ok;
        }

        inline void addIssue(CsvLoadResult& result, int line, IssueSeverity severity,
                             const QString& message) {
            result.issues.append(ParseIssue{line, severity, message});
        }

        inline bool isValidHeader(const QStringList& fields) {
            const QStringList& expected = expectedHeader();
            return fields.size() == expected.size() &&
                   std::equal(fields.cbegin(), fields.cend(), expected.cbegin(),
                              sameName);
        }

        inline void parseRow(const QStringList& fields, int lineNo,
                             CsvLoadResult& result) {
            if (fields.size() != ColumnCount) {
                addIssue(result, lineNo, IssueSeverity::Error,
                         tr("Строка пропущена: ожидалось полей — %1, получено — %2.")
                             .arg(ColumnCount)
                             .arg(fields.size()));
                return;
            }

            auto field = [&](Column c) { return fields.at(c).trimmed(); };

            QStringList errors;
            BomItem item;

            const QString buildingId = field(ColBuildingId);
            if (buildingId.isEmpty())
                errors << tr("пустой BuildingID");

            item.profileName = field(ColProfileName).simplified();
            if (item.profileName.isEmpty())
                errors << tr("пустой ProfileName");

            if (!parseDouble(field(ColLengthMm), item.lengthMm))
                errors << tr("Length_mm «%1» не является числом").arg(field(ColLengthMm));
            else if (!BomItem::isValidLength(item.lengthMm))
                errors << tr("Length_mm должно быть больше 0");

            if (!parseDouble(field(ColCost), item.cost))
                errors << tr("Cost «%1» не является числом").arg(field(ColCost));
            else if (!BomItem::isValidCost(item.cost))
                errors << tr("Cost не может быть отрицательной");

            if (!parseInt(field(ColQuantity), item.quantity))
                errors << tr("Quantity «%1» не является целым числом")
                              .arg(field(ColQuantity));
            else if (!BomItem::isValidQuantity(item.quantity))
                errors << tr("Quantity должно быть больше 0");

            if (!errors.isEmpty()) {
                addIssue(
                    result, lineNo, IssueSeverity::Error,
                    tr("Строка пропущена: %1.").arg(errors.join(QStringLiteral("; "))));
                return;
            }

            if (item.cost == 0.0)
                addIssue(result, lineNo, IssueSeverity::Warning,
                         tr("Нулевая стоимость позиции %1.").arg(item.profileName));

            Q_ASSERT(item.isValid());
            result.dataset.addItem(buildingId, item);
            ++result.acceptedRowCount;
        }

        inline CsvLoadResult parseStream(QTextStream& stream) {
            CsvLoadResult result;
            bool firstLine = true;
            int lineNo = 0;
            QString line;
            while (stream.readLineInto(&line)) {
                ++lineNo;
                if (lineNo == 1 && line.startsWith(QChar(0xFEFF)))  // UTF-8 BOM
                    line.remove(0, 1);
                if (line.trimmed().isEmpty())
                    continue;

                const QStringList fields = line.split(QChar(u','));
                // The header is optional: the first line is one if it starts with
                // "BuildingID".
                const bool isHeader =
                    firstLine &&
                    sameName(fields.constFirst(), expectedHeader().constFirst());
                firstLine = false;

                if (isHeader) {
                    if (!isValidHeader(fields)) {
                        result.fatalError = tr("Неверный заголовок CSV. Ожидается: %1")
                                                .arg(expectedHeader().join(u','));
                        break;
                    }
                    continue;
                }

                ++result.dataRowCount;
                parseRow(fields, lineNo, result);
            }

            if (result.fatalError.isEmpty()) {
                if (stream.status() != QTextStream::Ok)
                    result.fatalError = tr("Ошибка чтения данных.");
                else if (result.dataRowCount == 0)
                    result.fatalError = tr("Файл не содержит строк с данными.");
                else if (result.acceptedRowCount == 0)
                    result.fatalError =
                        tr("Не найдено ни одной корректной строки (ошибок: %1).")
                            .arg(result.errorCount());
            }
            if (!result.succeeded())
                result.dataset = {};
            return result;
        }
    }  // namespace

    int CsvLoadResult::errorCount() const noexcept {
        return static_cast<int>(std::count_if(
            issues.cbegin(), issues.cend(),
            [](const ParseIssue& i) { return i.severity == IssueSeverity::Error; }));
    }

    int CsvLoadResult::warningCount() const noexcept {
        return static_cast<int>(std::count_if(
            issues.cbegin(), issues.cend(),
            [](const ParseIssue& i) { return i.severity == IssueSeverity::Warning; }));
    }

    CsvLoadResult CsvBomReader::readFile(const QString& filePath) const {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            CsvLoadResult result;
            result.fatalError =
                tr("Не удалось открыть файл «%1»: %2").arg(filePath, file.errorString());
            return result;
        }
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);
        return parseStream(stream);
    }

    CsvLoadResult CsvBomReader::readText(const QString& text) const {
        QString copy = text;
        QTextStream stream(&copy, QIODevice::ReadOnly);
        return parseStream(stream);
    }
}  // namespace bom::infrastructure
