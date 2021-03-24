/*
   Copyright (C) 2019 Arto Hyvättinen

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
#ifndef ERACOMBO_H
#define ERACOMBO_H

#include <QComboBox>
#include <QDate>
#include "model/euro.h"

class EraCombo : public QComboBox
{
    Q_OBJECT
public:
    EraCombo(QWidget *parent = nullptr);

    int valittuEra() const;
    QVariantMap eraMap() const;

public slots:
    void asetaTili(int tili, int asiakas=0);
    void valitseUusiEra();
    void valitse(const QVariantMap& eraMap);

protected:
    void paivita();
    void vaihtui();

signals:
    void valittu(int eraid, Euro avoinna, const QString& selite, int kumppani);

private:        
    int eraId_=-1;
    QString eraNimi_;
    Euro eraSaldo_;
    QDate eranPaiva_;

    int tili_ = 0;
    int asiakas_ = 0;
    QString asiakasNimi_;

    bool paivitetaan_ = false;

};

#endif // ERACOMBO_H
