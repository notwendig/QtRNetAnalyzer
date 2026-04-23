// SPDX-License-Identifier: GPL-3.0-only
/*
 * QtRNetAnalyzer
 *
 * Copyright (c) 2026
 * ChatGPT (GPT-5.4 Thinking)
 * Jürgen Willi Sievers <JSievers@NadiSoft.de>
 */
#pragma once

#include <QStyledItemDelegate>

class LiveFrameDelegate final : public QStyledItemDelegate
{
    Q_OBJECT
  public:
    explicit LiveFrameDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
};
