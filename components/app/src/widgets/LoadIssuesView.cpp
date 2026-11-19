#include "widgets/LoadIssuesView.h"

#include <QApplication>
#include <QHeaderView>
#include <QStyle>

namespace bom::app {
    LoadIssuesView::LoadIssuesView(QWidget* parent) : QTableWidget(0, 3, parent) {
        setHorizontalHeaderLabels({tr("Строка"), tr("Тип"), tr("Сообщение")});
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        setSelectionBehavior(QAbstractItemView::SelectRows);
        setAlternatingRowColors(true);
        setWordWrap(false);
        verticalHeader()->setVisible(false);
        horizontalHeader()->setStretchLastSection(true);
    }

    void LoadIssuesView::setIssues(const QList<infrastructure::ParseIssue>& issues) {
        clearContents();
        setRowCount(static_cast<int>(issues.size()));

        const QIcon errorIcon = style()->standardIcon(QStyle::SP_MessageBoxCritical);
        const QIcon warningIcon = style()->standardIcon(QStyle::SP_MessageBoxWarning);

        for (int row = 0; row < issues.size(); ++row) {
            const infrastructure::ParseIssue& issue = issues[row];
            const bool isError = issue.severity == infrastructure::IssueSeverity::Error;

            auto* line = new QTableWidgetItem;
            line->setData(Qt::DisplayRole, issue.line);
            line->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            setItem(row, 0, line);
            setItem(row, 1,
                    new QTableWidgetItem(isError ? errorIcon : warningIcon,
                                         isError ? tr("Ошибка") : tr("Предупреждение")));
            auto* message = new QTableWidgetItem(issue.message);
            message->setToolTip(issue.message);
            setItem(row, 2, message);
        }
        resizeColumnToContents(0);
        resizeColumnToContents(1);
    }
}  // namespace bom::app
