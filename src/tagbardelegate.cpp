// SPDX-License-Identifier: GPL-3.0-only
/*
 * QtRNetAnalyzer
 *
 * Copyright (c) 2026
 * ChatGPT (GPT-5.4 Thinking)
 * Jürgen Willi Sievers <JSievers@NadiSoft.de>
 */
#include "tagbardelegate.h"

#include <QApplication>
#include <algorithm>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>

TagBarDelegate::TagBarDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void TagBarDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);
    opt.text.clear();

    const QWidget *widget = option.widget;
    QStyle *style = widget ? widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, widget);

    QStyleOptionButton cb;
    cb.state |= QStyle::State_Enabled;
    if (index.data(Qt::CheckStateRole).toInt() == Qt::Checked)
        cb.state |= QStyle::State_On;
    else
        cb.state |= QStyle::State_Off;

    const QRect indicator = style->subElementRect(QStyle::SE_CheckBoxIndicator, &cb, widget);
    cb.rect = QStyle::alignedRect(option.direction, Qt::AlignCenter, indicator.size(), option.rect);
    style->drawControl(QStyle::CE_CheckBox, &cb, painter, widget);
}

QSize TagBarDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize s = QStyledItemDelegate::sizeHint(option, index);
    s.setWidth(std::max(s.width(), 34));
    s.setHeight(std::max(s.height(), 24));
    return s;
}

bool TagBarDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &, const QModelIndex &index)
{
    if (!index.isValid() || !(index.flags() & Qt::ItemIsUserCheckable) || !(index.flags() & Qt::ItemIsEnabled))
        return false;

    if (event->type() == QEvent::MouseButtonRelease)
    {
        const auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() != Qt::LeftButton)
            return false;
    }
    else if (event->type() != QEvent::KeyPress)
    {
        return false;
    }

    const Qt::CheckState state = (index.data(Qt::CheckStateRole).toInt() == Qt::Checked)
                                      ? Qt::Unchecked
                                      : Qt::Checked;
    return model->setData(index, state, Qt::CheckStateRole);
}
