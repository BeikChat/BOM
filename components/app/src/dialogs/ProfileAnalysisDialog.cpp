#include "dialogs/ProfileAnalysisDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QVBoxLayout>
#include <algorithm>

#include "services/ProfileAnalyzerLibrary.h"

namespace bom::app {
    namespace {
        inline QString num(double v, int decimals) {
            return QLocale().toString(v, 'f', decimals);
        }

        inline QLabel* valueLabel(QWidget* parent) {
            auto* label = new QLabel(QStringLiteral("—"), parent);
            label->setTextInteractionFlags(Qt::TextSelectableByMouse);
            QFont font = label->font();
            font.setBold(true);
            label->setFont(font);
            return label;
        }
    }  // namespace

    ProfileAnalysisDialog::ProfileAnalysisDialog(
        ProfileAnalyzerLibrary& analyzer, const QList<core::ProfileSummary>& profiles,
        const QString& initialProfile, QWidget* parent)
        : QDialog(parent), m_analyzer(analyzer) {
        setWindowTitle(tr("Анализ профиля"));
        setMinimumWidth(420);

        QStringList names;
        for (const core::ProfileSummary& p : profiles) {
            m_totalLengthMByName.insert(p.profileName, p.totalLengthM);
            names.append(p.profileName);
        }

        std::sort(names.begin(), names.end());

        m_profileCombo = new QComboBox(this);
        m_profileCombo->setEditable(true);
        m_profileCombo->setInsertPolicy(QComboBox::NoInsert);
        m_profileCombo->addItems(names);
        m_profileCombo->setCurrentText(initialProfile);
        m_profileCombo->lineEdit()->setPlaceholderText(
            tr("Например: IPE200, HEA160, L150*5, RHS100x50x4"));

        auto* analyzeButton = new QPushButton(tr("Анализировать"), this);
        analyzeButton->setDefault(true);
        connect(analyzeButton, &QPushButton::clicked, this,
                &ProfileAnalysisDialog::runAnalysis);
        connect(m_profileCombo->lineEdit(), &QLineEdit::returnPressed, this,
                &ProfileAnalysisDialog::runAnalysis);

        auto* inputRow = new QHBoxLayout;
        inputRow->addWidget(m_profileCombo, 1);
        inputRow->addWidget(analyzeButton);

        auto* resultBox = new QGroupBox(tr("Результат"), this);
        auto* form = new QFormLayout(resultBox);
        m_type = valueLabel(resultBox);
        m_height = valueLabel(resultBox);
        m_weight = valueLabel(resultBox);
        m_totalWeight = valueLabel(resultBox);
        form->addRow(tr("Тип профиля:"), m_type);
        form->addRow(tr("Номинальная высота:"), m_height);
        form->addRow(tr("Вес 1 м:"), m_weight);
        form->addRow(tr("Масса во всех BOM:"), m_totalWeight);

        m_status = new QLabel(this);
        m_status->setWordWrap(true);

        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        auto* layout = new QVBoxLayout(this);
        layout->addLayout(inputRow);
        layout->addWidget(resultBox);
        layout->addWidget(m_status);
        layout->addWidget(buttons);

        if (!m_analyzer.isAvailable()) {
            analyzeButton->setEnabled(false);
            m_status->setStyleSheet(QStringLiteral("color: #c0392b;"));
            m_status->setDisabled(true);
            m_status->setText(m_analyzer.unavailableReason());
        } else if (!initialProfile.isEmpty()) {
            runAnalysis();
        }
    }

    void ProfileAnalysisDialog::setResultText(const QString& type, const QString& height,
                                              const QString& weight,
                                              const QString& totalWeight) {
        m_type->setText(type);
        m_height->setText(height);
        m_weight->setText(weight);
        m_totalWeight->setText(totalWeight);
    }

    void ProfileAnalysisDialog::runAnalysis() {
        const QString name = m_profileCombo->currentText().trimmed();
        const ProfileAnalysisResult result = m_analyzer.analyze(name);

        const QString dash = QStringLiteral("—");

        if (!result.has_value()) {
            auto error = result.error();

            setResultText(dash, dash, dash, dash);
            m_status->setStyleSheet(QStringLiteral("color: #c0392b;"));
            m_status->setText(
                tr("Код возврата %1: %2").arg(error.code).arg(error.message));
            return;
        }

        const ProfileAnalysis analyze = result.value();

        const QString type = analyze.profileType;
        const QString height = analyze.nominalHeightMm > 0
                                   ? tr("%1 мм").arg(num(analyze.nominalHeightMm, 1))
                                   : dash;

        QString totalWeight = tr("профиль не встречается в загруженных BOM");
        const auto it = m_totalLengthMByName.constFind(name);
        if (it != m_totalLengthMByName.cend()) {
            double weight = *it * analyze.weightPerMeterKg;
            totalWeight = tr("%1 кг (%2 м)").arg(num(weight, 1), num(*it, 1));
        }

        QString weight = tr("%1 кг/м").arg(num(analyze.weightPerMeterKg, 2));
        setResultText(type, height, weight, totalWeight);
        m_status->setStyleSheet({});
        m_status->setText(
            tr("Код возврата %1: %2").arg(analyze.code).arg(analyze.message));
    }
}  // namespace bom::app
