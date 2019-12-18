/*
   Copyright (C) 2017 Arto Hyvättinen

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <QSqlQuery>

#include "alvilmoitustenmodel.h"
#include "db/kirjanpito.h"

#include "alvsivu.h"

AlvIlmoitustenModel::AlvIlmoitustenModel(QObject *parent)
    : QAbstractTableModel(parent)
{

}

int AlvIlmoitustenModel::rowCount(const QModelIndex &/*parent*/) const
{
    return tiedot_.count();
}

int AlvIlmoitustenModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 4;
}

QVariant AlvIlmoitustenModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
    {
            return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    }
    else if( orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section) {
        case ALKAA:
            return tr("Alkaa");
        case PAATTYY:
            return tr("Päättyy");
        case ERAPVM:
            return tr("Eräpäivä");
        case VERO:
            return tr("Maksettava vero");
        }
    }
    return QVariant();
}

QVariant AlvIlmoitustenModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
    {
        return QVariant();
    }


    QVariantMap map = tiedot_.at( index.row() ).toMap();

    if( role == Qt::DisplayRole )
    {
        switch (index.column()) {
        case ALKAA:
            return map.value("kausialkaa").toDate();
        case PAATTYY:
            return map.value("kausipaattyy").toDate();
        case ERAPVM:
            return AlvSivu::erapaiva( map.value("kausipaattyy").toDate() );
        case VERO:
            return QString("%L1 €").arg( map.value("maksettava").toDouble() , 0,'f',2);
        }
    }
    else if( role == Qt::TextAlignmentRole)
    {
        if( index.column()==VERO)
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        else
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);

    }
    else if( role == TositeIdRooli )
        return map.value("id");
    else if( role == PaattyyRooli)
        return  map.value("kausipaattyy");
    else if( role == EraPvmRooli)
        return AlvSivu::erapaiva( map.value("kausipaattyy").toDate() );
    else if( role == AlkaaRooli)
        return map.value("kausialkaa");

    return QVariant();

}

qlonglong AlvIlmoitustenModel::marginaalialijaama(const QDate &paiva, int kanta) const
{
    for (QVariant item : tiedot_) {
        QVariantMap map = item.toMap();
        if( map.value("kausipaattyy").toDate() != paiva)
            continue;
        QVariantMap ajmap = map.value("marginaalialijaama").toMap();
        double aj = ajmap.value(QString::number(kanta/100.0,'f',2)).toDouble();
        return qRound64(aj * 100.0);
    }
    return 0;
}

void AlvIlmoitustenModel::lataa()
{
    KpKysely *kysely = kpk("/alv");
    if( !kysely)
        return;

    connect( kysely, &KpKysely::vastaus, this, &AlvIlmoitustenModel::dataSaapuu);
    kysely->kysy();
}

void AlvIlmoitustenModel::dataSaapuu(QVariant *data)
{
    beginResetModel();
    tiedot_.clear();
    tiedot_ = data->toList();
    endResetModel();
}


