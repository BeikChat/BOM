#pragma once

#include <QStyledItemDelegate>

namespace bom::app {
    // Delegate for the BOM tree (columns of BomTreeModel):
    //   * length is shown with the unit, "6 000 мм";
    //   * in-cell editors with the valid range of each field: QDoubleSpinBox for length
    //     (> 0, "мм") and cost (>= 0), QSpinBox for quantity (> 0).
    class BomItemDelegate : public QStyledItemDelegate {
        Q_OBJECT

    public:
        static constexpr double kMinLengthMm = 0.01;
        static constexpr double kMaxLengthMm = 1'000'000.0;
        static constexpr double kMaxCost = 1e12;
        static constexpr int kMaxQuantity = 1'000'000;

        using QStyledItemDelegate::QStyledItemDelegate;

        QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                              const QModelIndex& index) const override;
        void setEditorData(QWidget* editor, const QModelIndex& index) const override;
        void setModelData(QWidget* editor, QAbstractItemModel* model,
                          const QModelIndex& index) const override;

    protected:
        void initStyleOption(QStyleOptionViewItem* option,
                             const QModelIndex& index) const override;
    };
}  // namespace bom::app
