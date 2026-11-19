#pragma once

#include <QMainWindow>
#include "bom/core/analytics/BomAnalyzer.h"
#include "services/ProfileAnalyzerLibrary.h"

class QAction;
class QDockWidget;
class QLabel;
class QLineEdit;
class QSortFilterProxyModel;
class QTreeView;

namespace bom::app {
    class BomTreeModel;
    class LoadIssuesView;
    class SummaryPanel;

    class MainWindow : public QMainWindow {
        Q_OBJECT

    public:
        explicit MainWindow(QWidget* parent = nullptr);
        ~MainWindow() override;

        bool openFile(const QString& filePath);

    protected:
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dropEvent(QDropEvent* event) override;

    private slots:
        void onOpenTriggered();
        void onSaveReportTriggered();
        void onAnalyzeProfileTriggered();
        void onDatasetChanged();
        void onSelectionChanged();
        void onTreeContextMenu(const QPoint& pos);
        void copySelected();

    private:
        void createActions();
        void createMenusAndToolBar();
        void createCentralWidget();
        void createDocks();
        void createStatusBar();
        void recomputeReport();
        void updateActions();
        void updateTitle();
        void openProfileAnalysis(const QString& profileName);
        QString lastDirectory() const;
        void rememberDirectory(const QString& filePath);

        ProfileAnalyzerLibrary m_profileAnalyzer;
        core::AnalyticsReport m_report;
        QString m_currentFile;

        BomTreeModel* m_model = nullptr;
        QSortFilterProxyModel* m_proxy = nullptr;
        QTreeView* m_tree = nullptr;
        QLineEdit* m_filterEdit = nullptr;
        SummaryPanel* m_summary = nullptr;
        LoadIssuesView* m_issuesView = nullptr;
        QDockWidget* m_issuesDock = nullptr;
        QLabel* m_fileLabel = nullptr;
        QLabel* m_selectionLabel = nullptr;
        QLabel* m_totalLabel = nullptr;

        QAction* m_openAction = nullptr;
        QAction* m_saveReportAction = nullptr;
        QAction* m_analyzeProfileAction = nullptr;
        QAction* m_expandAllAction = nullptr;
        QAction* m_collapseAllAction = nullptr;
        QAction* m_copyAction = nullptr;
        QAction* m_exitAction = nullptr;
        QAction* m_aboutAction = nullptr;
    };
}  // namespace bom::app
