#include <QAbstractItemModelTester>
#include <QDoubleSpinBox>
#include <QSignalSpy>
#include <QSpinBox>
#include <QTest>

#include "TestData.h"
#include "models/BomItemDelegate.h"
#include "models/BomTreeModel.h"

using namespace bom;
using app::BomItemDelegate;
using app::BomTreeModel;

namespace {
    // Exposes the protected initStyleOption() to check the displayed text.
    struct DelegateProbe : BomItemDelegate {
        using BomItemDelegate::initStyleOption;
    };
}  // namespace

class TestBomTreeModel : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void structure();
    void aggregatesOnBomRows();
    void editingUpdatesDatasetAndAggregates();
    void invalidEditsAreRejected();
    void onlyPositionNumbersAreEditable();
    void fractionalLengthIsDisplayed();
    void delegateEditorsHaveRanges();
    void delegateEditCommitsValue();
    void delegateShowsLengthUnit();

private:
    BomTreeModel* m_model = nullptr;
    QAbstractItemModelTester* m_tester = nullptr;
};

void TestBomTreeModel::init() {
    m_model = new BomTreeModel;
    m_tester = new QAbstractItemModelTester(
        m_model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    m_model->setDataset(core::test::twoPrices());
}

void TestBomTreeModel::cleanup() {
    delete m_tester;
    delete m_model;
}

void TestBomTreeModel::structure() {
    QCOMPARE(m_model->rowCount(), 2);
    QCOMPARE(m_model->columnCount(), int(BomTreeModel::ColumnCount));
    const QModelIndex b1 = m_model->index(0, 0);
    QCOMPARE(m_model->nodeType(b1), BomTreeModel::NodeType::Bom);
    QCOMPARE(m_model->rowCount(b1), 2);
    const QModelIndex item = m_model->index(1, 0, b1);
    QCOMPARE(m_model->nodeType(item), BomTreeModel::NodeType::Item);
    QCOMPARE(item.parent(), b1);
    QCOMPARE(item.data().toString(), QStringLiteral("IPE240"));
    QCOMPARE(m_model->rowCount(item), 0);
}

void TestBomTreeModel::aggregatesOnBomRows() {
    const QModelIndex b1 = m_model->index(0, BomTreeModel::ColTotalCost);
    QCOMPARE(b1.data(Qt::EditRole).toDouble(), 1080.0);
    QCOMPARE(m_model->index(0, BomTreeModel::ColQuantity).data().toInt(), 3);
    QCOMPARE(
        m_model->index(0, BomTreeModel::ColCostPerMeter).data(Qt::EditRole).toDouble(),
        54.0);

    const QModelIndex cpm =
        m_model->index(0, BomTreeModel::ColCostPerMeter, m_model->index(0, 0));
    QCOMPARE(cpm.data(Qt::EditRole).toDouble(), 50.0);
}

void TestBomTreeModel::editingUpdatesDatasetAndAggregates() {
    QSignalSpy changed(m_model, &BomTreeModel::datasetChanged);
    const QModelIndex qty =
        m_model->index(0, BomTreeModel::ColQuantity, m_model->index(0, 0));

    QVERIFY(m_model->setData(qty, 4));
    QCOMPARE(changed.count(), 1);
    QCOMPARE(m_model->dataset().boms[0].items[0].quantity, 4);
    QCOMPARE(m_model->index(0, BomTreeModel::ColTotalCost).data(Qt::EditRole).toDouble(),
             1680.0);

    QVERIFY(m_model->setData(qty, 4));
    QCOMPARE(changed.count(), 1);
}

void TestBomTreeModel::invalidEditsAreRejected() {
    const QModelIndex bom = m_model->index(0, 0);
    QVERIFY(!m_model->setData(m_model->index(0, BomTreeModel::ColQuantity, bom), 0));
    QVERIFY(!m_model->setData(m_model->index(0, BomTreeModel::ColLength, bom), -1.0));
    QVERIFY(!m_model->setData(m_model->index(0, BomTreeModel::ColCost, bom),
                              QStringLiteral("abc")));
    QCOMPARE(m_model->dataset().boms[0].items[0].quantity, 2);
}

void TestBomTreeModel::onlyPositionNumbersAreEditable() {
    const QModelIndex bom = m_model->index(0, 0);
    QVERIFY(!(m_model->flags(m_model->index(0, BomTreeModel::ColQuantity)) &
              Qt::ItemIsEditable));
    QVERIFY(!(m_model->flags(m_model->index(0, BomTreeModel::ColName, bom)) &
              Qt::ItemIsEditable));
    QVERIFY(!(m_model->flags(m_model->index(0, BomTreeModel::ColTotalCost, bom)) &
              Qt::ItemIsEditable));
    for (int c :
         {BomTreeModel::ColLength, BomTreeModel::ColCost, BomTreeModel::ColQuantity})
        QVERIFY(m_model->flags(m_model->index(0, c, bom)) & Qt::ItemIsEditable);
}

void TestBomTreeModel::fractionalLengthIsDisplayed() {
    core::BomDataset ds;
    ds.boms.append(core::Bom{QStringLiteral("B1"),
                             {core::test::item("IPE200", 6000.5, 300, 1),
                              core::test::item("IPE200", 6000, 300, 1)}});
    m_model->setDataset(ds);
    const QModelIndex bom = m_model->index(0, 0);
    QCOMPARE(m_model->index(0, BomTreeModel::ColLength, bom).data().toString(),
             QLocale().toString(6000.5, 'f', 2));
    QCOMPARE(m_model->index(1, BomTreeModel::ColLength, bom).data().toString(),
             QLocale().toString(6000.0, 'f', 0));
}

void TestBomTreeModel::delegateEditorsHaveRanges() {
    BomItemDelegate delegate;
    QWidget parent;
    const QModelIndex bom = m_model->index(0, 0);

    auto* length = qobject_cast<QDoubleSpinBox*>(delegate.createEditor(
        &parent, {}, m_model->index(0, BomTreeModel::ColLength, bom)));
    QVERIFY(length);
    QCOMPARE(length->suffix(), QStringLiteral(" мм"));
    QCOMPARE(length->minimum(), BomItemDelegate::kMinLengthMm);
    QCOMPARE(length->maximum(), BomItemDelegate::kMaxLengthMm);

    auto* cost = qobject_cast<QDoubleSpinBox*>(delegate.createEditor(
        &parent, {}, m_model->index(0, BomTreeModel::ColCost, bom)));
    QVERIFY(cost);
    QCOMPARE(cost->minimum(), 0.0);

    auto* quantity = qobject_cast<QSpinBox*>(delegate.createEditor(
        &parent, {}, m_model->index(0, BomTreeModel::ColQuantity, bom)));
    QVERIFY(quantity);
    QCOMPARE(quantity->minimum(), 1);
    QCOMPARE(quantity->maximum(), BomItemDelegate::kMaxQuantity);
}

void TestBomTreeModel::delegateEditCommitsValue() {
    BomItemDelegate delegate;
    QWidget parent;
    const QModelIndex length =
        m_model->index(0, BomTreeModel::ColLength, m_model->index(0, 0));
    auto* editor =
        qobject_cast<QDoubleSpinBox*>(delegate.createEditor(&parent, {}, length));
    QVERIFY(editor);

    delegate.setEditorData(editor, length);
    QCOMPARE(editor->value(), 6000.0);
    editor->setValue(6500.0);
    delegate.setModelData(editor, m_model, length);
    QCOMPARE(m_model->dataset().boms[0].items[0].lengthMm, 6500.0);
}

void TestBomTreeModel::delegateShowsLengthUnit() {
    DelegateProbe delegate;
    const QModelIndex bom = m_model->index(0, 0);

    QStyleOptionViewItem option;
    delegate.initStyleOption(&option, m_model->index(0, BomTreeModel::ColLength, bom));
    QCOMPARE(option.text, QLocale().toString(6000.0, 'f', 0) + QStringLiteral(" мм"));

    QStyleOptionViewItem bomOption;
    delegate.initStyleOption(&bomOption, m_model->index(0, BomTreeModel::ColLength));
    QVERIFY(bomOption.text.isEmpty());
}

QTEST_MAIN(TestBomTreeModel)
#include "tst_bom_tree_model.moc"
