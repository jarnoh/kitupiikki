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

#ifndef TILINMUOKKAUSDIALOG_H
#define TILINMUOKKAUSDIALOG_H

#include <QDialog>

#include "ui_tilinmuokkaus.h"
#include "db/tilimodel.h"

class TilinMuokkausDialog : public QDialog
{
    Q_OBJECT
public:    
    TilinMuokkausDialog(TiliModel *model, QModelIndex index = QModelIndex());
    ~TilinMuokkausDialog();

protected:
    void lataa();

protected slots:
    void veroEnablePaivita();
    void otsikkoTasoPaivita();

protected:
    Ui::tilinmuokkausDialog *ui;
    TiliModel *model_;
    QModelIndex index_;


};

#endif // TILINMUOKKAUSDIALOG_H
