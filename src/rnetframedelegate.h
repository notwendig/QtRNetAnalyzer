#pragma once

#include <QStyledItemDelegate>

class RNetFrameDelegate final : public QStyledItemDelegate
{
    Q_OBJECT
  public:
    explicit RNetFrameDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
};
