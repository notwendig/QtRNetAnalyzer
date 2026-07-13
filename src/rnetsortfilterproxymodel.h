#pragma once

#include <QSortFilterProxyModel>

// Sorting proxy for the R-Net table.
//
// The R-Net model intentionally returns user-facing strings for many columns
// (hex CAN IDs, formatted timestamps, textual names). QSortFilterProxyModel's
// default string comparison sorts those lexicographically, which is wrong for
// numeric columns such as Count, #, ID, Ext, RTR, and Timestamp.
//
// This proxy keeps DisplayRole untouched and applies semantic column sorting
// based on the visible header text. It is deliberately independent from the
// concrete RNetFrameModel enum names so older local branches still build.
class RNetSortFilterProxyModel : public QSortFilterProxyModel
{
public:
    explicit RNetSortFilterProxyModel(QObject *parent = nullptr);

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    enum class SortKind {
        Text,
        CheckState,
        Integer,
        Double,
        HexInteger
    };

    SortKind sortKindForColumn(int sourceColumn) const;
    double numericValue(const QModelIndex &index, SortKind kind, bool *ok) const;
    QString textValue(const QModelIndex &index) const;
};
