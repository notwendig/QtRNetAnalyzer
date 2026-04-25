#include "rnetframedelegate.h"

#include <QAbstractItemModel>
#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionButton>
#include <QStyleOptionViewItem>

RNetFrameDelegate::RNetFrameDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void RNetFrameDelegate::paint(QPainter *painter,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    const QWidget *widget = opt.widget;
    QStyle *style = widget ? widget->style() : QApplication::style();

    const QVariant checkState = index.data(Qt::CheckStateRole);
    if (checkState.isValid())
    {
        style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, widget);

        QStyleOptionButton cb;
        cb.state = QStyle::State_Enabled;
        cb.state |= (checkState.toInt() == Qt::Checked)
                        ? QStyle::State_On
                        : QStyle::State_Off;

        const QRect indicator = style->subElementRect(QStyle::SE_CheckBoxIndicator, &cb, widget);
        cb.rect = QRect(opt.rect.center().x() - indicator.width() / 2,
                        opt.rect.center().y() - indicator.height() / 2,
                        indicator.width(),
                        indicator.height());

        style->drawControl(QStyle::CE_CheckBox, &cb, painter, widget);
        return;
    }

    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, widget);

    painter->save();

    QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, widget);
    textRect.adjust(4, 0, -4, 0);

    const QVariant bg = index.data(Qt::BackgroundRole);
    if (bg.canConvert<QBrush>() && !(opt.state & QStyle::State_Selected))
        painter->fillRect(opt.rect, qvariant_cast<QBrush>(bg));

    if (opt.state & QStyle::State_Selected)
    {
        painter->fillRect(opt.rect, opt.palette.highlight());
        painter->setPen(opt.palette.highlightedText().color());
    }
    else
    {
        painter->setPen(opt.palette.text().color());
    }

    const QString text = index.data(Qt::DisplayRole).toString();
    const Qt::Alignment alignment = static_cast<Qt::Alignment>(
        index.data(Qt::TextAlignmentRole).toInt());

    painter->drawText(textRect, alignment, text);
    painter->restore();
}

bool RNetFrameDelegate::editorEvent(QEvent *event,
                                    QAbstractItemModel *model,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index)
{
    Q_UNUSED(option)

    if (!model || !index.isValid() || !index.data(Qt::CheckStateRole).isValid())
        return QStyledItemDelegate::editorEvent(event, model, option, index);

    if (event->type() == QEvent::MouseButtonRelease) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() != Qt::LeftButton)
            return false;
    } else if (event->type() != QEvent::KeyPress) {
        return false;
    }

    const Qt::CheckState oldState = static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt());
    const Qt::CheckState newState = oldState == Qt::Checked ? Qt::Unchecked : Qt::Checked;
    return model->setData(index, newState, Qt::CheckStateRole);
}

QSize RNetFrameDelegate::sizeHint(const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
{
    return QStyledItemDelegate::sizeHint(option, index);
}
