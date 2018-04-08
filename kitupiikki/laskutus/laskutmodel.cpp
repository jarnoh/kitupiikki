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

#include "laskutmodel.h"
#include "db/kirjanpito.h"
#include <QSqlQuery>


LaskutModel::LaskutModel(QObject *parent) :
    QAbstractTableModel(parent)
{

}

int LaskutModel::rowCount(const QModelIndex & /* parent */) const
{
    return laskut.count();
}

int LaskutModel::columnCount(const QModelIndex & /* parent */) const
{
    return 6;
}

QVariant LaskutModel::data(const QModelIndex &item, int role) const
{
    if( !item.isValid())
        return QVariant();

    AvoinLasku lasku = laskut.value(item.row());

    if( role == Qt::DisplayRole || role == Qt::EditRole)
    {

        switch (item.column())
        {
        case NUMERO:
            return lasku.viitenro;
        case PVM:
            return lasku.pvm;
        case ERAPVM:
            if( lasku.kirjausperuste == LaskuModel::KATEISLASKU ||  lasku.json.luku("Hyvityslasku"))
                return QString();
            return lasku.erapvm;
        case SUMMA:
            if( role == Qt::DisplayRole)
                return QString("%L1 €").arg(lasku.summaSnt / 100.0,0,'f',2);
            else
               return lasku.summaSnt;
        case MAKSAMATTA:
            if( role == Qt::DisplayRole)
            {
                if( lasku.avoinSnt)
                    return QString("%L1 €").arg(lasku.avoinSnt / 100.0,0,'f',2);
                else
                    return QVariant();  // Nollalle tyhjää
            }
            else
                return lasku.avoinSnt;
        case ASIAKAS:
            return lasku.asiakas;
        }
    }
    else if( role == Qt::TextAlignmentRole)
    {
        if( item.column() == SUMMA || item.column() == MAKSAMATTA )
            return QVariant( Qt::AlignRight | Qt::AlignVCenter);
    }
    else if( role == Qt::TextColorRole && item.column() == ERAPVM)
    {
        if( kp()->paivamaara().daysTo( lasku.erapvm) < 0 && lasku.avoinSnt )
            return QColor(Qt::red);
    }
    else if( role == Qt::DecorationRole && item.column() == PVM)
    {
        if( lasku.json.luku("Hyvityslasku") )
            return QIcon(":/pic/poista.png");

        switch (lasku.kirjausperuste) {
        case LaskuModel::SUORITEPERUSTE:
            return QIcon(":/pic/suorite.png");
        case LaskuModel::LASKUTUSPERUSTE:
            return QIcon(":/pic/kirje.png");
        case LaskuModel::MAKSUPERUSTE :
            return QIcon(":/pic/euro.png");
        case LaskuModel::KATEISLASKU :
            return QIcon(":/pic/kateinen.png");

        }
    }

    else if( role == TositeRooli)
        return lasku.tosite;
    else if( role == AvoinnaRooli)
        return lasku.avoinSnt;
    else if( role == JsonRooli )
        return lasku.json.toJson();
    else if( role == ViiteRooli)
        return lasku.viitenro;
    else if( role == AsiakasRooli)
        return lasku.asiakas;
    else if( role == LiiteRooli)
        return lasku.json.str("Liite");
    else if( role == HyvitysLaskuRooli)
        return lasku.json.luku("Hyvityslasku");
    else if( role == KirjausPerusteRooli)
        return lasku.kirjausperuste;

    return QVariant();
}

QVariant LaskutModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter);
    else if( role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section) {
        case NUMERO: return tr("Viitenumero");
        case PVM: return tr("Laskun pvm");
        case ERAPVM: return tr("Eräpvm");
        case SUMMA: return tr("Summa");
        case MAKSAMATTA: return tr("Maksamatta");
        case ASIAKAS: return tr("Asiakas");
        }
    }
    return QVariant();
}

AvoinLasku LaskutModel::laskunTiedot(int indeksi) const
{
    return laskut.value(indeksi);
}

void LaskutModel::lataaAvoimet()
{
    paivita( AVOIMET) ;
}

void LaskutModel::paivita(int valinta, QDate mista, QDate mihin)
{
    QString kysely = "SELECT id, pvm, tili, debetsnt, kreditsnt, eraid, viite, erapvm, json, tosite, asiakas FROM vienti "
                     "WHERE viite IS NOT NULL AND iban IS NULL ";

    beginResetModel();
    laskut.clear();
    QSqlQuery query( kysely );

    while( query.next())
    {
        JsonKentta json( query.value("json").toByteArray()  );
        QDate laskupvm = json.date("Laskupvm");
        if( laskupvm < mista || laskupvm > mihin)
            continue;

        TaseEra era( query.value("eraid").toInt());
        if( valinta == AVOIMET && !era.saldoSnt )
            continue;
        if( valinta == ERAANTYNEET && ( !era.saldoSnt || query.value("erapvm").toDate() > kp()->paivamaara() ))
            continue;

        // Tämä lasku kelpaa ;)
        AvoinLasku lasku;
        lasku.viitenro = query.value("viite").toInt();
        lasku.pvm = laskupvm;
        lasku.erapvm = query.value("erapvm").toDate();
        lasku.summaSnt = query.value("debetSnt").toInt();
        lasku.avoinSnt = era.saldoSnt;
        lasku.asiakas = query.value("asiakas").toString();
        lasku.tosite = query.value("tosite").toInt();
        lasku.kirjausperuste =  json.luku("Kirjausperuste");
        lasku.json = json;
        laskut.append(lasku);
    }
    endResetModel();
}

void LaskutModel::maksa(int indeksi, int senttia)
{
    laskut[indeksi].avoinSnt -= senttia;
    if( laskut.value(indeksi).avoinSnt == 0)
    {
        beginRemoveRows(QModelIndex(), indeksi, indeksi);
        laskut.removeAt(indeksi);
        endRemoveRows();
    }
}

QString LaskutModel::bicIbanilla(const QString &iban)
{

    // Pitää olla suomalainen IBAN
    if( !iban.startsWith("FI"))
        return QString();

    // Elokuun 2017 tilanteen mukaan
    QString tunnus = iban.mid(4);

    if( tunnus.startsWith("405") || tunnus.startsWith("497"))
        return "HELSFIHH";  // Aktia Pankki
    else if( tunnus.startsWith('8') )
        return "DABAFIHH";  // Danske Bank
    else if( tunnus.startsWith("34"))
        return "DABAFIHX";
    else if( tunnus.startsWith("31"))
        return "HANDFIHH";  // Handelsbanken
    else if( tunnus.startsWith('1') || tunnus.startsWith('2'))
        return "NDEAFIHH";  // Nordea
    else if( tunnus.startsWith('5'))
        return "OKOYFIHH";  // Osuuspankit
    else if( tunnus.startsWith("39") || tunnus.startsWith("36"))
        return "SBANFIHH";  // S-Pankki
    else if( tunnus.startsWith('6'))
        return "AABAFI22";  // Ålandsbanken
    else if( tunnus.startsWith("47") )
        return "POPFFI22"; // POP Pankit
    else if( tunnus.startsWith("715") || tunnus.startsWith('4'))
        return "ITELFIHH"; // Säästöpankkiryhmä
    else if( tunnus.startsWith("717"))
        return "BIGKFIH1";
    else if( tunnus.startsWith("713"))
        return "CITIFIHX";
    else if( tunnus.startsWith("37"))
        return "DNBAFIHX";
    else if( tunnus.startsWith("799"))
        return "HOLVFIHH";
    else if( tunnus.startsWith("33"))
        return "ESSEFIHX";
    else if( tunnus.startsWith("38"))
        return "SWEDFIHH";

    // Tuntematon pankkikoodi
    return QString();
}
