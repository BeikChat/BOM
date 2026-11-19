#pragma once

#include <QAbstractItemModel>

#include "bom/core/domain/Bom.h"

namespace bom::app {
    class BomTreeModel : public QAbstractItemModel {
        Q_OBJECT

    public:
        enum Column {
            ColName,
            ColLength,
            ColCost,
            ColQuantity,
            ColTotalCost,
            ColCostPerMeter,
            ColumnCount
        };

        enum class NodeType { Invalid, Bom, Item };
        Q_ENUM(NodeType)

        explicit BomTreeModel(QObject* parent = nullptr);

        void setDataset(core::BomDataset dataset);
        const core::BomDataset& dataset() const noexcept { return m_dataset; }

        NodeType nodeType(const QModelIndex& index) const;

        QModelIndex index(int row, int column,
                          const QModelIndex& parent = {}) const override;
        QModelIndex parent(const QModelIndex& child) const override;
        int rowCount(const QModelIndex& parent = {}) const override;
        int columnCount(const QModelIndex& parent = {}) const override;

        QVariant data(const QModelIndex& index,
                      int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex& index, const QVariant& value,
                     int role = Qt::EditRole) override;

        Qt::ItemFlags flags(const QModelIndex& index) const override;
        QVariant headerData(int section, Qt::Orientation orientation,
                            int role = Qt::DisplayRole) const override;

    signals:
        void datasetChanged();

    private:
        QVariant value(const QModelIndex& index) const;
        const core::BomItem& itemAt(const QModelIndex& index) const;

        core::BomDataset m_dataset;
    };
}  // namespace bom::app
