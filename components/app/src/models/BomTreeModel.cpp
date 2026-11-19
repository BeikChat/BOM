#include "models/BomTreeModel.h"

#include <QBrush>
#include <QFont>
#include <QLocale>
#include <cmath>

namespace bom::app {
    namespace {
        inline bool isWholeNumber(double value) {
            return std::abs(value - std::round(value)) < 1e-9;
        }
    }  // namespace

    BomTreeModel::BomTreeModel(QObject* parent) : QAbstractItemModel(parent) {}

    void BomTreeModel::setDataset(core::BomDataset dataset) {
        beginResetModel();
        m_dataset = std::move(dataset);
        endResetModel();
    }

    BomTreeModel::NodeType BomTreeModel::nodeType(const QModelIndex& index) const {
        if (!index.isValid() || index.model() != this)
            return NodeType::Invalid;

        if (index.internalId() == 0)
            return index.row() < m_dataset.boms.size() ? NodeType::Bom
                                                       : NodeType::Invalid;

        return NodeType::Item;
    }

    QModelIndex BomTreeModel::index(int row, int column,
                                    const QModelIndex& parent) const {
        if (row < 0 || column < 0 || column >= ColumnCount)
            return {};

        if (!parent.isValid())
            return row < m_dataset.boms.size() ? createIndex(row, column, quintptr(0))
                                               : QModelIndex{};
        if (nodeType(parent) != NodeType::Bom ||
            row >= m_dataset.boms.at(parent.row()).items.size())
            return {};

        return createIndex(row, column, quintptr(parent.row()) + 1);
    }

    QModelIndex BomTreeModel::parent(const QModelIndex& child) const {
        if (nodeType(child) != NodeType::Item)
            return {};

        return createIndex(static_cast<int>(child.internalId() - 1), 0, quintptr(0));
    }

    int BomTreeModel::rowCount(const QModelIndex& parent) const {
        if (!parent.isValid())
            return static_cast<int>(m_dataset.boms.size());

        if (parent.column() != 0 || nodeType(parent) != NodeType::Bom)
            return 0;

        return static_cast<int>(m_dataset.boms.at(parent.row()).items.size());
    }

    int BomTreeModel::columnCount(const QModelIndex&) const {
        return ColumnCount;
    }

    const core::BomItem& BomTreeModel::itemAt(const QModelIndex& index) const {
        return m_dataset.boms.at(static_cast<qsizetype>(index.internalId() - 1))
            .items.at(index.row());
    }

    QVariant BomTreeModel::value(const QModelIndex& index) const {
        switch (nodeType(index)) {
            case NodeType::Bom: {
                const core::Bom& bom = m_dataset.boms.at(index.row());
                switch (index.column()) {
                    case ColName:
                        return bom.buildingId;
                    case ColQuantity:
                        return bom.totalQuantity();
                    case ColTotalCost:
                        return bom.totalCost();
                    case ColCostPerMeter:
                        return bom.averageCostPerMeter();
                    default:
                        return {};
                }
            }
            case NodeType::Item: {
                const core::BomItem& item = itemAt(index);
                switch (index.column()) {
                    case ColName:
                        return item.profileName;
                    case ColLength:
                        return item.lengthMm;
                    case ColCost:
                        return item.cost;
                    case ColQuantity:
                        return item.quantity;
                    case ColTotalCost:
                        return item.totalCost();
                    case ColCostPerMeter:
                        return item.costPerMeter();
                    default:
                        return {};
                }
            }
            case NodeType::Invalid:
                break;
        }
        return {};
    }

    QVariant BomTreeModel::data(const QModelIndex& index, int role) const {
        const NodeType type = nodeType(index);
        if (type == NodeType::Invalid)
            return {};

        switch (role) {
            case Qt::DisplayRole: {
                const QVariant v = value(index);
                if (v.typeId() != QMetaType::Double)
                    return v;

                const double number = v.toDouble();
                const bool wholeLength =
                    index.column() == ColLength && isWholeNumber(number);
                return QLocale().toString(number, 'f', wholeLength ? 0 : 2);
            }
            case Qt::EditRole:
                return value(index);
            case Qt::TextAlignmentRole:
                return index.column() == ColName
                           ? QVariant()
                           : QVariant(Qt::AlignRight | Qt::AlignVCenter);
            case Qt::FontRole:
                if (type == NodeType::Bom) {
                    QFont font;
                    font.setBold(true);
                    return font;
                }
                return {};
            case Qt::ForegroundRole:
                if (type == NodeType::Item && itemAt(index).cost == 0.0 &&
                    (index.column() == ColCost || index.column() == ColTotalCost))
                    return QBrush(Qt::darkYellow);

                return {};
            default:
                return {};
        }
    }

    bool BomTreeModel::setData(const QModelIndex& index, const QVariant& value,
                               int role) {
        if (role != Qt::EditRole || !(flags(index) & Qt::ItemIsEditable))
            return false;

        const int bomRow = static_cast<int>(index.internalId() - 1);
        core::BomItem& item = m_dataset.boms[bomRow].items[index.row()];

        bool ok = false;
        if (index.column() == ColQuantity) {
            const int v = value.toInt(&ok);
            if (!ok || !core::BomItem::isValidQuantity(v))
                return false;

            if (v == item.quantity)
                return true;

            item.quantity = v;
        } else {
            const double v = value.toDouble(&ok);
            const bool isCost = index.column() == ColCost;
            if (!ok || !(isCost ? core::BomItem::isValidCost(v)
                                : core::BomItem::isValidLength(v)))
                return false;

            double& field = isCost ? item.cost : item.lengthMm;
            if (v == field)
                return true;

            field = v;
        }

        // The position and the aggregates of its BOM.
        emit dataChanged(index.siblingAtColumn(0),
                         index.siblingAtColumn(ColumnCount - 1));
        emit dataChanged(this->index(bomRow, 0), this->index(bomRow, ColumnCount - 1));
        emit datasetChanged();
        return true;
    }

    Qt::ItemFlags BomTreeModel::flags(const QModelIndex& index) const {
        if (!index.isValid())
            return Qt::NoItemFlags;

        Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if (nodeType(index) == NodeType::Item &&
            (index.column() == ColLength || index.column() == ColCost ||
             index.column() == ColQuantity))
            f |= Qt::ItemIsEditable;
        return f;
    }

    QVariant BomTreeModel::headerData(int section, Qt::Orientation orientation,
                                      int role) const {
        if (orientation != Qt::Horizontal)
            return {};

        switch (role) {
            case Qt::TextAlignmentRole:
                return section == ColName ? QVariant(Qt::AlignLeft | Qt::AlignVCenter)
                                          : QVariant(Qt::AlignRight | Qt::AlignVCenter);
            case Qt::ToolTipRole:
                switch (section) {
                    case ColLength:
                        return tr("Длина отрезка, мм");
                    case ColCost:
                        return tr("Цена за целый отрезок (не за метр)");
                    case ColTotalCost:
                        return tr(
                            "Стоимость позиции: Цена × Количество; для BOM — сумма "
                            "по позициям");
                    case ColCostPerMeter:
                        return tr(
                            "Цена / Длина × 1000; для BOM — средневзвешенная по "
                            "длине");
                    default:
                        return {};
                }
            case Qt::DisplayRole:
                switch (section) {
                    case ColName:
                        return tr("BOM / Профиль");
                    case ColLength:
                        return tr("Длина");
                    case ColCost:
                        return tr("Цена отрезка");
                    case ColQuantity:
                        return tr("Кол-во");
                    case ColTotalCost:
                        return tr("Стоимость");
                    case ColCostPerMeter:
                        return tr("Цена за метр");
                    default:
                        return {};
                }
            default:
                return {};
        }
    }
}  // namespace bom::app
