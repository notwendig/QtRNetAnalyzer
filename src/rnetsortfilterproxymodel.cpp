#include "rnetsortfilterproxymodel.h"

#include <QAbstractItemModel>
#include <QCollator>
#include <QString>
#include <QVariant>

#include <limits>

namespace {

QString normalizedHeader(const QVariant &value)
{
    return value.toString().trimmed().toLower();
}

bool parseDisplayNumber(const QString &text, double *out)
{
    bool ok = false;
    const double value = text.trimmed().toDouble(&ok);
    if (ok && out)
        *out = value;
    return ok;
}

bool parseDisplayInteger(const QString &text, double *out)
{
    bool ok = false;
    const qulonglong value = text.trimmed().toULongLong(&ok, 10);
    if (ok && out)
        *out = static_cast<double>(value);
    return ok;
}

bool parseDisplayHex(const QString &text, double *out)
{
    QString s = text.trimmed();
    if (s.startsWith(QStringLiteral("0X")))
        s = QStringLiteral("0x") + s.mid(2);

    bool ok = false;
    const qulonglong value = s.toULongLong(&ok, 0);
    if (ok && out)
        *out = static_cast<double>(value);
    return ok;
}

QCollator &sortCollator()
{
    static QCollator collator;
    static bool initialized = false;
    if (!initialized) {
        collator.setCaseSensitivity(Qt::CaseInsensitive);
        collator.setNumericMode(true);
        initialized = true;
    }
    return collator;
}

} // namespace

RNetSortFilterProxyModel::RNetSortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
}

bool RNetSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const SortKind kind = sortKindForColumn(left.column());

    if (kind != SortKind::Text) {
        bool leftOk = false;
        bool rightOk = false;
        const double l = numericValue(left, kind, &leftOk);
        const double r = numericValue(right, kind, &rightOk);

        if (leftOk && rightOk) {
            if (l == r)
                return textValue(left).compare(textValue(right), Qt::CaseInsensitive) < 0;
            return l < r;
        }
        if (leftOk != rightOk)
            return leftOk;
    }

    return sortCollator().compare(textValue(left), textValue(right)) < 0;
}

RNetSortFilterProxyModel::SortKind RNetSortFilterProxyModel::sortKindForColumn(int sourceColumn) const
{
    const QAbstractItemModel *m = sourceModel();
    if (!m)
        return SortKind::Text;

    const QString h = normalizedHeader(m->headerData(sourceColumn, Qt::Horizontal, Qt::DisplayRole));

    if (h == QStringLiteral("plot"))
        return SortKind::CheckState;
    if (h == QStringLiteral("#") || h == QStringLiteral("row") || h == QStringLiteral("nr") || h == QStringLiteral("no"))
        return SortKind::Integer;
    if (h == QStringLiteral("count") || h == QStringLiteral("anzahl"))
        return SortKind::Integer;
    if (h == QStringLiteral("id") || h == QStringLiteral("can-id") || h == QStringLiteral("can id"))
        return SortKind::HexInteger;
    if (h == QStringLiteral("ext") || h == QStringLiteral("rtr"))
        return SortKind::Integer;
    if (h == QStringLiteral("timestamp") || h == QStringLiteral("time") || h == QStringLiteral("zeit"))
        return SortKind::Double;

    return SortKind::Text;
}

double RNetSortFilterProxyModel::numericValue(const QModelIndex &index, SortKind kind, bool *ok) const
{
    if (ok)
        *ok = false;

    if (!index.isValid())
        return 0.0;

    if (kind == SortKind::CheckState) {
        const QVariant state = index.data(Qt::CheckStateRole);
        bool conv = false;
        const int value = state.toInt(&conv);
        if (conv) {
            if (ok)
                *ok = true;
            return static_cast<double>(value);
        }
    }

    const QVariant sortData = index.data(Qt::UserRole);
    bool variantOk = false;
    const double variantNumber = sortData.toDouble(&variantOk);
    if (variantOk) {
        if (ok)
            *ok = true;
        return variantNumber;
    }

    const QString display = index.data(Qt::DisplayRole).toString();
    double value = 0.0;
    bool parsed = false;

    switch (kind) {
    case SortKind::Integer:
        parsed = parseDisplayInteger(display, &value) || parseDisplayNumber(display, &value);
        break;
    case SortKind::Double:
        parsed = parseDisplayNumber(display, &value);
        break;
    case SortKind::HexInteger:
        parsed = parseDisplayHex(display, &value);
        break;
    case SortKind::CheckState:
    case SortKind::Text:
        break;
    }

    if (parsed && ok)
        *ok = true;
    return value;
}

QString RNetSortFilterProxyModel::textValue(const QModelIndex &index) const
{
    return index.data(Qt::DisplayRole).toString();
}
