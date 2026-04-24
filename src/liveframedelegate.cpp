#include "liveframedelegate.h"

#include <QApplication>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionViewItem>

LiveFrameDelegate::LiveFrameDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void LiveFrameDelegate::paint(QPainter *painter,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    const QWidget *widget = opt.widget;
    QStyle *style = widget ? widget->style() : QApplication::style();

    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, widget);

    painter->save();

    QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, widget);
    textRect.adjust(4, 0, -4, 0);

    const QVariant bg = index.data(Qt::BackgroundRole);
    if (bg.canConvert<QBrush>() && !(opt.state & QStyle::State_Selected)) {
        painter->fillRect(opt.rect, qvariant_cast<QBrush>(bg));
    }

    if (opt.state & QStyle::State_Selected) {
        painter->fillRect(opt.rect, opt.palette.highlight());
        painter->setPen(opt.palette.highlightedText().color());
    } else {
        painter->setPen(opt.palette.text().color());
    }

    const QString text = index.data(Qt::DisplayRole).toString();
    const Qt::Alignment alignment = static_cast<Qt::Alignment>(
        index.data(Qt::TextAlignmentRole).toInt()
        );

    painter->drawText(textRect, alignment, text);
    painter->restore();
}

QSize LiveFrameDelegate::sizeHint(const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
{
    return QStyledItemDelegate::sizeHint(option, index);
}
