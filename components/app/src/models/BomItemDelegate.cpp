#include "models/BomItemDelegate.h"

#include <QDoubleSpinBox>
#include <QSpinBox>
#include <algorithm>

#include "models/BomTreeModel.h"

namespace bom::app {
    namespace {
        template <typename SpinBox>
        inline SpinBox* createSpinBox(QWidget* parent) {
            auto* editor = new SpinBox(parent);
            editor->setFrame(false);
            editor->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            editor->setAccelerated(true);
            return editor;
        }

        inline QDoubleSpinBox* createDoubleSpinBox(QWidget* parent, double min, double max,
                                            double step) {
            auto* editor = createSpinBox<QDoubleSpinBox>(parent);
            editor->setDecimals(2);
            editor->setRange(min, max);
            editor->setSingleStep(step);
            return editor;
        }

        inline QSpinBox* createSpinBox(QWidget* parent, double min, double max) {
            auto* editor = createSpinBox<QSpinBox>(parent);
            editor->setRange(min, max);
            return editor;
        }

    }  // namespace

    QWidget* BomItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                                           const QModelIndex& index) const {
        switch (index.column()) {
            case BomTreeModel::ColLength: {
                QDoubleSpinBox* editor =
                    createDoubleSpinBox(parent, kMinLengthMm, kMaxLengthMm, 100.0);
                editor->setSuffix(tr(" мм"));
                return editor;
            }
            case BomTreeModel::ColCost:
                return createDoubleSpinBox(parent, 0.0, kMaxCost, 10.0);
            case BomTreeModel::ColQuantity:
                return createSpinBox(parent, 1, kMaxQuantity);
            default:
                return QStyledItemDelegate::createEditor(parent, option, index);
        }
    }

    void BomItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
        const QVariant value = index.data(Qt::EditRole);
        // The range is widened to a loaded value above the limit, so that opening and
        // closing the editor never clamps the data.
        if (auto* spinBox = qobject_cast<QDoubleSpinBox*>(editor)) {
            spinBox->setMaximum(std::max(spinBox->maximum(), value.toDouble()));
            spinBox->setValue(value.toDouble());
        } else if (auto* intSpinBox = qobject_cast<QSpinBox*>(editor)) {
            intSpinBox->setMaximum(std::max(intSpinBox->maximum(), value.toInt()));
            intSpinBox->setValue(value.toInt());
        } else {
            QStyledItemDelegate::setEditorData(editor, index);
        }
    }

    void BomItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                       const QModelIndex& index) const {
        QStyledItemDelegate::setModelData(editor, model, index);
    }

    void BomItemDelegate::initStyleOption(QStyleOptionViewItem* option,
                                          const QModelIndex& index) const {
        QStyledItemDelegate::initStyleOption(option, index);
        if (index.column() == BomTreeModel::ColLength && !option->text.isEmpty())
            option->text = tr("%1 мм").arg(option->text);
    }

}  // namespace bom::app
