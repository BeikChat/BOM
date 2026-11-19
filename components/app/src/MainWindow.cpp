#include "MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyle>
#include <QToolBar>
#include <QTreeView>
#include <QVBoxLayout>
#include <algorithm>

#include "dialogs/ProfileAnalysisDialog.h"
#include "infrastructure/CsvBomReader.h"
#include "infrastructure/ReportJsonWriter.h"
#include "models/BomItemDelegate.h"
#include "models/BomTreeModel.h"
#include "widgets/LoadIssuesView.h"
#include "widgets/SummaryPanel.h"

namespace bom::app {
    namespace {
        const char* const kSettingsLastDir = "lastDirectory";
        const char* const kSettingsGeometry = "mainWindow/geometry";
        const char* const kSettingsState = "mainWindow/state";

        inline QString money(double v) {
            return QLocale().toString(v, 'f', 2);
        }

        inline bool isCsvUrl(const QUrl& url) {
            return url.isLocalFile() && url.toLocalFile().endsWith(QStringLiteral(".csv"),
                                                                   Qt::CaseInsensitive);
        }
    }  // namespace

    MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
        setAcceptDrops(true);
        resize(1280, 760);

        createActions();
        createCentralWidget();
        createDocks();
        createMenusAndToolBar();
        createStatusBar();

        QSettings settings;
        restoreGeometry(settings.value(kSettingsGeometry).toByteArray());
        restoreState(settings.value(kSettingsState).toByteArray());

        recomputeReport();
        updateTitle();
        updateActions();
    }

    MainWindow::~MainWindow() {
        QSettings settings;
        settings.setValue(kSettingsGeometry, saveGeometry());
        settings.setValue(kSettingsState, saveState());
    }

    void MainWindow::createActions() {
        QStyle* s = style();

        m_openAction = new QAction(s->standardIcon(QStyle::SP_DialogOpenButton),
                                   tr("&Открыть CSV…"), this);
        m_openAction->setShortcut(QKeySequence::Open);
        m_openAction->setToolTip(tr(
            "Загрузить BOM из CSV-файла (Ctrl+O)."));
        connect(m_openAction, &QAction::triggered, this, &MainWindow::onOpenTriggered);

        m_saveReportAction = new QAction(s->standardIcon(QStyle::SP_DialogSaveButton),
                                         tr("&Сохранить отчёт…"), this);
        m_saveReportAction->setShortcut(QKeySequence::Save);
        m_saveReportAction->setToolTip(
            tr("Экспортировать аналитический отчёт в JSON (Ctrl+S)"));
        connect(m_saveReportAction, &QAction::triggered, this,
                &MainWindow::onSaveReportTriggered);

        m_analyzeProfileAction =
            new QAction(s->standardIcon(QStyle::SP_FileDialogContentsView),
                        tr("&Анализ профиля…"), this);
        m_analyzeProfileAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
        m_analyzeProfileAction->setToolTip(
            tr("Определить тип, высоту и вес профиля (Ctrl+I)"));
        connect(m_analyzeProfileAction, &QAction::triggered, this,
                &MainWindow::onAnalyzeProfileTriggered);

        m_expandAllAction = new QAction(tr("Развернуть всё"), this);
        m_collapseAllAction = new QAction(tr("Свернуть всё"), this);

        m_copyAction = new QAction(tr("Копировать"), this);
        m_copyAction->setShortcut(QKeySequence::Copy);
        m_copyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(m_copyAction, &QAction::triggered, this, &MainWindow::copySelected);

        m_exitAction = new QAction(tr("В&ыход"), this);
        m_exitAction->setShortcut(QKeySequence::Quit);
        connect(m_exitAction, &QAction::triggered, this, &QWidget::close);

        m_aboutAction = new QAction(tr("О программе"), this);
        connect(m_aboutAction, &QAction::triggered, this, [this] {
            QMessageBox::about(
                this, tr("О программе"),
                tr("<b>%1 %2</b><br>Загрузка, визуализация и анализ BOM-данных.<br>"
                   "Qt %3")
                    .arg(QCoreApplication::applicationName(),
                         QCoreApplication::applicationVersion(),
                         QString::fromLatin1(qVersion())));
        });
    }

    void MainWindow::createCentralWidget() {
        m_model = new BomTreeModel(this);
        connect(m_model, &BomTreeModel::datasetChanged, this,
                &MainWindow::onDatasetChanged);

        m_proxy = new QSortFilterProxyModel(this);
        m_proxy->setSourceModel(m_model);
        m_proxy->setSortRole(Qt::EditRole);
        m_proxy->setFilterKeyColumn(BomTreeModel::ColName);
        m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
        m_proxy->setRecursiveFilteringEnabled(true);
        m_proxy->setAutoAcceptChildRows(true);
        m_proxy->setSortLocaleAware(true);

        m_tree = new QTreeView(this);
        m_tree->setModel(m_proxy);
        m_tree->setItemDelegate(new BomItemDelegate(m_tree));
        m_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_tree->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tree->setAlternatingRowColors(true);
        m_tree->setUniformRowHeights(true);
        m_tree->setEditTriggers(QAbstractItemView::DoubleClicked |
                                QAbstractItemView::EditKeyPressed |
                                QAbstractItemView::SelectedClicked);
        m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
        m_tree->header()->setSortIndicator(-1, Qt::AscendingOrder);
        m_tree->setSortingEnabled(true);
        m_tree->header()->setSectionsMovable(true);
        m_tree->header()->setStretchLastSection(false);
        m_tree->header()->setDefaultAlignment(Qt::AlignCenter);
        m_tree->addAction(m_copyAction);

        connect(m_tree, &QTreeView::customContextMenuRequested, this,
                &MainWindow::onTreeContextMenu);
        connect(m_tree->selectionModel(), &QItemSelectionModel::selectionChanged, this,
                &MainWindow::onSelectionChanged);
        connect(m_expandAllAction, &QAction::triggered, m_tree, &QTreeView::expandAll);
        connect(m_collapseAllAction, &QAction::triggered, m_tree,
                &QTreeView::collapseAll);

        m_filterEdit = new QLineEdit(this);
        m_filterEdit->setPlaceholderText(tr("Фильтр по BOM или профилю…"));
        m_filterEdit->setClearButtonEnabled(true);
        connect(m_filterEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
            m_proxy->setFilterFixedString(text);
            m_tree->expandAll();
        });

        auto* left = new QWidget(this);
        auto* leftLayout = new QVBoxLayout(left);
        leftLayout->setContentsMargins(0, 0, 0, 0);
        leftLayout->addWidget(m_filterEdit);
        leftLayout->addWidget(m_tree);

        m_summary = new SummaryPanel(this);

        auto* splitter = new QSplitter(Qt::Horizontal, this);
        splitter->addWidget(left);
        splitter->addWidget(m_summary);
        splitter->setStretchFactor(0, 3);
        splitter->setStretchFactor(1, 2);
        splitter->setChildrenCollapsible(false);
        setCentralWidget(splitter);
    }

    void MainWindow::createMenusAndToolBar() {
        QMenu* file = menuBar()->addMenu(tr("&Файл"));
        file->addAction(m_openAction);
        file->addAction(m_saveReportAction);
        file->addSeparator();
        file->addAction(m_exitAction);

        QMenu* edit = menuBar()->addMenu(tr("&Правка"));
        edit->addAction(m_copyAction);

        QMenu* tools = menuBar()->addMenu(tr("&Инструменты"));
        tools->addAction(m_analyzeProfileAction);

        QMenu* view = menuBar()->addMenu(tr("&Вид"));
        view->setObjectName(QStringLiteral("viewMenu"));
        view->addAction(m_expandAllAction);
        view->addAction(m_collapseAllAction);
        view->addSeparator();
        view->addAction(m_issuesDock->toggleViewAction());

        QMenu* help = menuBar()->addMenu(tr("&Справка"));
        help->addAction(m_aboutAction);

        QToolBar* toolBar = addToolBar(tr("Основная панель"));
        toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        toolBar->addAction(m_openAction);
        toolBar->addAction(m_saveReportAction);
        toolBar->addSeparator();
        toolBar->addAction(m_analyzeProfileAction);
        toolBar->addSeparator();
        toolBar->addAction(m_expandAllAction);
        toolBar->addAction(m_collapseAllAction);
    }

    void MainWindow::createDocks() {
        m_issuesView = new LoadIssuesView(this);
        m_issuesDock = new QDockWidget(tr("Журнал загрузки"), this);
        m_issuesDock->setWidget(m_issuesView);
        addDockWidget(Qt::BottomDockWidgetArea, m_issuesDock);
        m_issuesDock->hide();
    }

    void MainWindow::createStatusBar() {
        m_fileLabel = new QLabel(this);
        m_selectionLabel = new QLabel(this);
        m_totalLabel = new QLabel(this);

        statusBar()->addPermanentWidget(m_selectionLabel);
        statusBar()->addPermanentWidget(m_totalLabel);
        statusBar()->addPermanentWidget(m_fileLabel);
    }

    bool MainWindow::openFile(const QString& filePath) {
        const QString fileName = QFileInfo(filePath).fileName();

        QApplication::setOverrideCursor(Qt::WaitCursor);
        infrastructure::CsvLoadResult result = infrastructure::CsvBomReader().readFile(filePath);
        QApplication::restoreOverrideCursor();

        m_issuesView->setIssues(result.issues);
        m_issuesDock->setWindowTitle(
            tr("Журнал загрузки — %1 (ошибок: %2, предупреждений: %3)")
                .arg(fileName)
                .arg(result.errorCount())
                .arg(result.warningCount()));
        if (!result.issues.isEmpty())
            m_issuesDock->show();

        if (!result.succeeded()) {
            QMessageBox::critical(
                this, tr("Ошибка загрузки"),
                tr("Не удалось загрузить «%1».\n\n%2").arg(fileName, result.fatalError));
            return false;
        }

        m_currentFile = filePath;
        rememberDirectory(filePath);
        m_model->setDataset(std::move(result.dataset));
        m_tree->expandAll();

        for (int c = 0; c < BomTreeModel::ColumnCount; ++c)
            m_tree->resizeColumnToContents(c);

        m_tree->setColumnWidth(BomTreeModel::ColName,
                               std::max(m_tree->columnWidth(BomTreeModel::ColName), 160));

        setWindowModified(false);
        recomputeReport();
        updateTitle();
        updateActions();

        statusBar()->showMessage(
            tr("Загружено строк: %1, пропущено: %2, предупреждений: %3")
                .arg(result.acceptedRowCount)
                .arg(result.rejectedRowCount())
                .arg(result.warningCount()),
            10000);

        if (result.errorCount() > 0) {
            QMessageBox::warning(
                this, tr("Загрузка с ошибками"),
                tr("Файл загружен частично: пропущено строк — %1 из %2.\n"
                   "Подробности — в панели «Журнал загрузки».")
                    .arg(result.rejectedRowCount())
                    .arg(result.dataRowCount));
        }
        return true;
    }

    void MainWindow::onOpenTriggered() {
        const QString path =
            QFileDialog::getOpenFileName(this, tr("Открыть CSV"), lastDirectory(),
                                         tr("CSV-файлы (*.csv);;Все файлы (*)"));
        if (!path.isEmpty())
            openFile(path);
    }

    void MainWindow::onSaveReportTriggered() {
        if (m_model->dataset().isEmpty())
            return;

        const QString baseName = QFileInfo(m_currentFile).completeBaseName();
        const QString suggested =
            QDir(lastDirectory()).filePath(baseName + QStringLiteral("_report.json"));
        const QString path = QFileDialog::getSaveFileName(this, tr("Сохранить отчёт"),
                                                          suggested, tr("JSON (*.json)"));
        if (path.isEmpty())
            return;

        infrastructure::ReportMetadata meta;
        meta.sourceFile = QFileInfo(m_currentFile).fileName();
        meta.generatedAt = QDateTime::currentDateTimeUtc();
        meta.generator = QStringLiteral("%1 %2").arg(
            QCoreApplication::applicationName(), QCoreApplication::applicationVersion());
        meta.editedAfterLoad = isWindowModified();

        QString error;
        if (!infrastructure::ReportJsonWriter::writeFile(path, m_report, meta, &error)) {
            QMessageBox::critical(this, tr("Ошибка сохранения"), error);
            return;
        }
        rememberDirectory(path);
        statusBar()->showMessage(
            tr("Отчёт сохранён: %1").arg(QDir::toNativeSeparators(path)), 10000);
    }

    void MainWindow::recomputeReport() {
        m_report = core::BomAnalyzer::analyze(m_model->dataset());
        if (m_model->dataset().isEmpty())
            m_summary->clear();
        else
            m_summary->setReport(m_report);
        m_totalLabel->setText(tr("Итого: %1").arg(money(m_report.summary.totalCost)));
    }

    void MainWindow::onDatasetChanged() {
        setWindowModified(true);
        recomputeReport();
        onSelectionChanged();
        updateActions();
    }

    void MainWindow::onSelectionChanged() {
        const qsizetype count = m_tree->selectionModel()->selectedRows().size();
        m_selectionLabel->setText(count > 0 ? tr("Выделено строк: %1").arg(count)
                                            : QString());
        updateActions();
    }

    void MainWindow::onTreeContextMenu(const QPoint& pos) {
        const QModelIndex proxyIndex = m_tree->indexAt(pos);
        const QModelIndex source = m_proxy->mapToSource(proxyIndex);

        QMenu menu(this);
        switch (m_model->nodeType(source)) {
            case BomTreeModel::NodeType::Item: {
                const QString profile =
                    source.siblingAtColumn(BomTreeModel::ColName).data().toString();
                menu.addAction(m_analyzeProfileAction->icon(),
                               tr("Анализ профиля «%1»…").arg(profile), this,
                               [this, profile] { openProfileAnalysis(profile); });
                menu.addSeparator();
                break;
            }
            case BomTreeModel::NodeType::Bom: {
                const QModelIndex first = proxyIndex.siblingAtColumn(0);
                const bool expanded = m_tree->isExpanded(first);
                menu.addAction(
                    expanded ? tr("Свернуть") : tr("Развернуть"), this,
                    [this, first, expanded] { m_tree->setExpanded(first, !expanded); });
                menu.addSeparator();
                break;
            }
            case BomTreeModel::NodeType::Invalid:
                break;
        }

        menu.addAction(m_copyAction);
        menu.addSeparator();
        menu.addAction(m_expandAllAction);
        menu.addAction(m_collapseAllAction);
        menu.exec(m_tree->viewport()->mapToGlobal(pos));
    }

    void MainWindow::copySelected() {
        QModelIndexList rows = m_tree->selectionModel()->selectedRows(0);
        if (rows.isEmpty())
            return;

        std::sort(rows.begin(), rows.end(),
                  [this](const QModelIndex& a, const QModelIndex& b) {
                      return m_tree->visualRect(a).top() < m_tree->visualRect(b).top();
                  });

        QStringList lines;
        QStringList header;
        for (int c = 0; c < BomTreeModel::ColumnCount; ++c)
            header << m_proxy->headerData(c, Qt::Horizontal).toString();

        lines << header.join(u'\t');

        for (const QModelIndex& row : std::as_const(rows)) {
            QStringList cells;
            for (int c = 0; c < BomTreeModel::ColumnCount; ++c)
                cells << row.siblingAtColumn(c).data(Qt::DisplayRole).toString();
            lines << cells.join(u'\t');
        }

        QApplication::clipboard()->setText(lines.join(u'\n'));
        statusBar()->showMessage(tr("Скопировано строк: %1").arg(rows.size()), 4000);
    }

    void MainWindow::onAnalyzeProfileTriggered() {
        const QModelIndex source = m_proxy->mapToSource(m_tree->currentIndex());
        const QString profile =
            m_model->nodeType(source) == BomTreeModel::NodeType::Item
                ? source.siblingAtColumn(BomTreeModel::ColName).data().toString()
                : QString();
        openProfileAnalysis(profile);
    }

    void MainWindow::openProfileAnalysis(const QString& profileName) {
        ProfileAnalysisDialog dialog(m_profileAnalyzer, m_report.profiles, profileName, this);
        dialog.exec();
    }

    void MainWindow::updateActions() {
        const bool hasData = !m_model->dataset().isEmpty();
        const bool hasSelection = m_tree->selectionModel()->hasSelection();
        m_saveReportAction->setEnabled(hasData);
        m_expandAllAction->setEnabled(hasData);
        m_collapseAllAction->setEnabled(hasData);
        m_copyAction->setEnabled(hasSelection);
    }

    void MainWindow::updateTitle() {
        const QString app = QCoreApplication::applicationName();
        if (m_currentFile.isEmpty()) {
            setWindowTitle(app);
            m_fileLabel->setText(tr("Файл не загружен"));
        } else {
            setWindowTitle(QStringLiteral("%1[*] — %2")
                               .arg(QFileInfo(m_currentFile).fileName(), app));
            m_fileLabel->setText(QDir::toNativeSeparators(m_currentFile));
        }
    }

    QString MainWindow::lastDirectory() const {
        const QString fallback =
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        return QSettings().value(kSettingsLastDir, fallback).toString();
    }

    void MainWindow::rememberDirectory(const QString& filePath) {
        QSettings().setValue(kSettingsLastDir, QFileInfo(filePath).absolutePath());
    }

    void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
        const QList<QUrl> urls = event->mimeData()->urls();
        if (urls.size() == 1 && isCsvUrl(urls.first()))
            event->acceptProposedAction();
    }

    void MainWindow::dropEvent(QDropEvent* event) {
        const QList<QUrl> urls = event->mimeData()->urls();
        if (urls.size() == 1 && isCsvUrl(urls.first())) {
            event->acceptProposedAction();
            openFile(urls.first().toLocalFile());
        }
    }

}  // namespace bom::app
