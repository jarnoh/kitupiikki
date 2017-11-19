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

#include <QDebug>
#include <QTimer>
#include <cmath>
#include "kirjausapuridialog.h"
#include "ui_kirjausapuridialog.h"

KirjausApuriDialog::KirjausApuriDialog(TositeModel *tositeModel, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::KirjausApuriDialog),
    model(tositeModel)
{
    ui->setupUi(this);

    // Alussa suurin valinnoista jää piiloon
    ui->nettoSpin->setVisible(false);
    ui->nettoLabel->setVisible(false);
    ui->alvCombo->setVisible(false);
    ui->alvlajiLabel->setVisible(false);
    ui->alvSpin->setVisible(false);
    ui->alvprossaLabel->setVisible(false);

    ui->eraLabel->setVisible(false);
    ui->taseEraCombo->setVisible(false);

    ui->vastaTaseEraLabel->setVisible(false);
    ui->vastaTaseEraCombo->setVisible(false);


    // ValintaTab ylälaidassa kirjauksen tyypin valintaan
    ui->valintaTab->addTab(QIcon(":/pic/lisaa.png"),"Tulo");
    ui->valintaTab->addTab(QIcon(":/pic/poista.png"),"Meno");
    ui->valintaTab->addTab(QIcon(":/pic/siirra.png"),"Siirto");
    ui->valintaTab->setCurrentIndex(SIIRTO);

    ui->kohdennusCombo->setModel( kp()->kohdennukset() );
    ui->kohdennusCombo->setModelColumn( KohdennusModel::NIMI );

    ui->taseEraCombo->setModel( &eraModelTilille);
    ui->vastaTaseEraCombo->setModel(&eraModelVastaTilille);

    verofiltteri.setSourceModel( kp()->alvTyypit() );
    verofiltteri.setFilterRole( VerotyyppiModel::KoodiTekstiRooli);
    ui->alvCombo->setModel( &verofiltteri );

    ui->ehdotusView->setModel( &ehdotus );

    ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled( false );

    connect( ui->valintaTab, SIGNAL(currentChanged(int)), this, SLOT(valilehtiVaihtui(int)));

    connect( ui->tiliEdit, SIGNAL(editingFinished()), this, SLOT(tiliTaytetty()));

    connect( ui->vastatiliEdit, SIGNAL(textChanged(QString)), this, SLOT(vastaTiliMuuttui()));
    connect( ui->alvCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(alvLajiMuuttui()));
    connect( ui->euroSpin, SIGNAL(editingFinished()), this, SLOT(laskeNetto()));
    connect( ui->nettoSpin, SIGNAL(editingFinished()), this, SLOT(laskeBrutto()));
    connect( ui->alvSpin, SIGNAL(editingFinished()), this, SLOT(laskeVerolla()));
    connect( ui->vaihdaNappi, SIGNAL(clicked(bool)), this, SLOT(ehdota()));
    connect(ui->seliteEdit, SIGNAL(editingFinished()), this, SLOT(ehdota()));
    connect(ui->pvmDate, SIGNAL(editingFinished()), this, SLOT(ehdota()));

    // Hakee tositteen tiedoista esitäytöt
    ui->pvmDate->setDate( model->pvm() );
    ui->seliteEdit->setText( model->otsikko());

    ui->tiliEdit->valitseTiliNumerolla( model->tositelaji().json()->luku("Oletustili"));

    // Jos kirjataan tiliotteelle, niin tiliotetili lukitaan vastatiliksi
    if( model->tiliotetili())
    {
        ui->vastatiliEdit->valitseTiliIdlla( model->tiliotetili());
        ui->vastatiliEdit->setEnabled(false);
    }
    else
    {
        // Suodatetaan tili valintojen mukaan
        int kirjauslaji = model->tositelaji().json()->luku("Kirjaustyyppi");
        if( kirjauslaji == TositelajiModel::OSTOLASKUT)
        {
            ui->valintaTab->setCurrentIndex(MENO);
            ui->valintaTab->setTabEnabled(TULO, false);
            ui->valintaTab->setTabEnabled(SIIRTO,false);
        }
        else if(kirjauslaji == TositelajiModel::MYYNTILASKUT)
        {
            ui->valintaTab->setCurrentIndex(TULO);
            ui->valintaTab->setTabEnabled(MENO, false);
            ui->valintaTab->setTabEnabled(SIIRTO, false);
        }

        // Jos tositelajille on määritelty oletusvastatili, esitäytetään sekin
        ui->vastatiliEdit->valitseTiliNumerolla( model->tositelaji().json()->luku("Vastatili") );
    }
    QTimer::singleShot(0, this, SLOT(korjaaSarakeLeveydet()));

}

KirjausApuriDialog::~KirjausApuriDialog()
{
    delete ui;
}

void KirjausApuriDialog::tiliTaytetty()
{
    // Jos tilillä on vastatili, niin täytetään se
    Tili tili = kp()->tilit()->tiliNumerolla(  ui->tiliEdit->valittuTilinumero() );

    if( tili.onkoValidi() && tili.numero() && ui->tiliEdit->text().length() > 5)
    {
        if( tili.json()->luku("Vastatili")  )
        {
            ui->vastatiliEdit->valitseTiliNumerolla( tili.json()->luku("Vastatili") );
            vastaTiliMuuttui();
        }
        ui->alvSpin->setValue( tili.json()->luku("AlvProsentti"));
        ui->alvCombo->setCurrentIndex( ui->alvCombo->findData( tili.json()->luku("AlvLaji") ) );

        if( tili.onkoMenotili() )
        {
            ui->valintaTab->setCurrentIndex(MENO);
        }
        else if( tili.onkoTulotili() )
        {
            ui->valintaTab->setCurrentIndex(TULO);
        }
        else if( tili.onkoTasetili() && !tili.onkoPoistettavaTaseTili())
        {
            ui->valintaTab->setCurrentIndex(SIIRTO);
        }

        ui->eraLabel->setVisible( tili.onkoTaseEraSeurattava());
        ui->taseEraCombo->setVisible(tili.onkoTaseEraSeurattava());
        if( tili.onkoTaseEraSeurattava() )
        {
            eraModelTilille.lataa( tili );
            ui->taseEraCombo->setCurrentIndex(0);
        }

        // Tilityyppi määrää, mitkä välilehdet mahdollisia!
        ui->valintaTab->setTabEnabled(MENO, tili.onkoMenotili() || tili.onkoPoistettavaTaseTili());
        ui->valintaTab->setTabEnabled(TULO, tili.onkoTulotili() || tili.onkoPoistettavaTaseTili());
        ui->valintaTab->setTabEnabled(SIIRTO, tili.onkoTasetili());

        // Verotuksen oletuskäsittely
        ui->alvCombo->setCurrentIndex( ui->alvCombo->findData( tili.json()->luku("AlvLaji") ) );
        ui->alvSpin->setValue( tili.json()->luku("AlvProsentti"));
        valilehtiVaihtui( ui->valintaTab->currentIndex());  // Jotta veroruudut näytetään!

    }
    ehdota();
}

void KirjausApuriDialog::laskeBrutto()
{
    double netto = ui->nettoSpin->value();
    if( netto )
    {
        nettoEur = ui->nettoSpin->value();
        bruttoEur = 0;

        ui->euroSpin->setValue( (100.0 + ui->alvSpin->value()) * netto / 100.0 );
    }
    ehdota();
}

void KirjausApuriDialog::laskeVerolla()
{
    if( nettoEur )
        ui->euroSpin->setValue( (100.0 + ui->alvSpin->value() )* nettoEur / 100.0 );
    else if(bruttoEur)
        ui->nettoSpin->setValue( (100.0 * bruttoEur ) /  (100.0 + ui->alvSpin->value())  );
    ehdota();
}

void KirjausApuriDialog::alvLajiMuuttui()
{
    int alvlaji = ui->alvCombo->currentData().toInt();

    if( ui->alvCombo->currentData(VerotyyppiModel::NollaLajiRooli).toBool())
    {
        ui->alvSpin->setValue(0);
        ui->alvSpin->setEnabled(false);
        laskeNetto();
    }
    else
        ui->alvSpin->setEnabled(true);

    ui->alvSpin->setVisible( alvlaji );
    ui->alvprossaLabel->setVisible(alvlaji);

    ehdota();
}

void KirjausApuriDialog::vastaTiliMuuttui()
{
    Tili vastatili = kp()->tilit()->tiliNumerolla( ui->vastatiliEdit->valittuTilinumero());
    ui->vastaTaseEraLabel->setVisible( vastatili.onkoTaseEraSeurattava());
    ui->vastaTaseEraCombo->setVisible( vastatili.onkoTaseEraSeurattava());
    if( vastatili.onkoTaseEraSeurattava() )
    {
        eraModelVastaTilille.lataa( vastatili );
        ui->vastaTaseEraCombo->setCurrentIndex(0);
    }
    ehdota();
}

void KirjausApuriDialog::ehdota()
{
    ehdotus.tyhjaa();

    Tili tili = kp()->tilit()->tiliNumerolla( ui->tiliEdit->valittuTilinumero() );
    Tili vastatili = kp()->tilit()->tiliNumerolla( ui->vastatiliEdit->valittuTilinumero());
    int nettoSnt = std::round(ui->nettoSpin->value() * 100.0);
    int bruttoSnt = std::round(ui->euroSpin->value() * 100.0);
    int alvprosentti = ui->alvSpin->value();
    int alvkoodi = ui->alvCombo->currentData(VerotyyppiModel::KoodiRooli).toInt();


    switch ( ui->valintaTab->currentIndex()) {
    case TULO:
        // Tulojen (myynti) kirjaus  per myyntitili an tasetili
        if( tili.onkoTulotili() || tili.onkoPoistettavaTaseTili())
        {
            VientiRivi tulorivi = uusiEhdotusRivi(tili);
            tulorivi.kreditSnt = nettoSnt;
            tulorivi.kohdennus = kp()->kohdennukset()->kohdennus(ui->kohdennusCombo->currentData(KohdennusModel::IdRooli).toInt());
            tulorivi.alvprosentti = alvprosentti;
            tulorivi.alvkoodi = alvkoodi;
            tulorivi.eraId = ui->taseEraCombo->currentData(EranValintaModel::EraIdRooli).toInt();
            ehdotus.lisaaVienti(tulorivi);
        }
        if(alvkoodi == AlvKoodi::MYYNNIT_NETTO && kp()->tilit()->tiliTyyppikoodilla("BL").onkoValidi() )
        {

            VientiRivi verorivi = uusiEhdotusRivi( kp()->tilit()->tiliTyyppikoodilla("BL"));
            verorivi.kreditSnt = bruttoSnt - nettoSnt;
            verorivi.alvprosentti = alvprosentti;
            verorivi.alvkoodi = AlvKoodi::ALVKIRJAUS + alvkoodi;
            ehdotus.lisaaVienti(verorivi);
        }
        if( vastatili.onkoTasetili() )
        {
            VientiRivi taserivi = uusiEhdotusRivi();
            taserivi.tili = vastatili;
            if( alvkoodi == AlvKoodi::MYYNNIT_NETTO )
                taserivi.debetSnt = bruttoSnt;
            else
                taserivi.debetSnt = nettoSnt;
            taserivi.eraId = ui->vastaTaseEraCombo->currentData(EranValintaModel::EraIdRooli).toInt();
            ehdotus.lisaaVienti(taserivi);
        }
        break;

    case MENO:
        // Menojen (osto) kirjaus per tasetili an ostotili
        if( tili.onkoMenotili() || tili.onkoPoistettavaTaseTili() )
        {
            VientiRivi menorivi = uusiEhdotusRivi(tili);
            menorivi.debetSnt = nettoSnt;
            menorivi.kohdennus = kp()->kohdennukset()->kohdennus(ui->kohdennusCombo->currentData(KohdennusModel::IdRooli).toInt());
            menorivi.alvprosentti = alvprosentti;
            menorivi.alvkoodi = alvkoodi;
            menorivi.eraId = ui->taseEraCombo->currentData().toInt();
            ehdotus.lisaaVienti( menorivi );

        }
        if( alvkoodi && kp()->tilit()->tiliTyyppikoodilla("AL").onkoValidi())
        {
            VientiRivi verorivi = uusiEhdotusRivi( kp()->tilit()->tiliTyyppikoodilla("AL"));
            verorivi.debetSnt = bruttoSnt - nettoSnt;
            verorivi.alvprosentti = alvprosentti;
            verorivi.alvkoodi = AlvKoodi::ALVVAHENNYS + alvkoodi;
            ehdotus.lisaaVienti(verorivi);

            if( (alvkoodi == AlvKoodi::YHTEISOHANKINNAT_PALVELUT || alvkoodi==AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
                alvkoodi == AlvKoodi::RAKENNUSPALVELU_OSTO) && kp()->tilit()->tiliTyyppikoodilla("BL").onkoValidi() )
            {
                VientiRivi lisarivi = uusiEhdotusRivi( kp()->tilit()->tiliTyyppikoodilla("BL"));
                lisarivi.kreditSnt = bruttoSnt - nettoSnt;
                lisarivi.alvprosentti = alvprosentti;
                lisarivi.alvkoodi = AlvKoodi::ALVKIRJAUS + alvkoodi;
                ehdotus.lisaaVienti(lisarivi);
            }
        }
        if( vastatili.onkoTasetili())
        {
            VientiRivi taserivi = uusiEhdotusRivi(vastatili);
            if( alvkoodi == AlvKoodi::OSTOT_NETTO)
                taserivi.kreditSnt = bruttoSnt;
            else
                taserivi.kreditSnt = nettoSnt;
            taserivi.eraId = ui->vastaTaseEraCombo->currentData().toInt();
            ehdotus.lisaaVienti(taserivi);
        }
        break;

    case SIIRTO:
        if( tili.onkoValidi() )
        {
            VientiRivi rivi = uusiEhdotusRivi(tili);

            if( ui->vaihdaNappi->isChecked())
                rivi.kreditSnt = bruttoSnt;
            else
                rivi.debetSnt = bruttoSnt;

            rivi.eraId = ui->taseEraCombo->currentData(KohdennusModel::IdRooli).toInt();
            ehdotus.lisaaVienti(rivi);
        }
        if( vastatili.onkoValidi())
        {
            VientiRivi rivi = uusiEhdotusRivi(vastatili);
            if( ui->vaihdaNappi->isChecked())
                rivi.debetSnt = bruttoSnt;
            else
                rivi.kreditSnt = bruttoSnt;
            rivi.eraId = ui->vastaTaseEraCombo->currentData(KohdennusModel::IdRooli).toInt();
            ehdotus.lisaaVienti(rivi);
        }

        break;
    }

    // Netto näytetään jos vero
    ui->nettoLabel->setVisible(ui->alvSpin->value());
    ui->nettoSpin->setVisible( ui->alvSpin->value());

    ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled( ehdotus.onkoKelpo() );

    // Poisto näytetään jos kirjataan tasaeräpoistotilille
    ui->poistoLabel->setVisible( tili.tyyppi() == "APT");
    ui->poistoSpin->setVisible(tili.tyyppi() == "APT");

}


void KirjausApuriDialog::valilehtiVaihtui(int indeksi)
{
    ui->kohdennusLabel->setVisible( indeksi != SIIRTO && kp()->kohdennukset()->rowCount(QModelIndex()) > 1);
    ui->kohdennusCombo->setVisible( indeksi != SIIRTO && kp()->kohdennukset()->rowCount(QModelIndex()) > 1);
    ui->vaihdaNappi->setVisible(indeksi == SIIRTO );


    bool verot = kp()->asetukset()->onko("AlvVelvollinen") && indeksi != SIIRTO;

    ui->alvlajiLabel->setVisible(verot);
    ui->alvCombo->setVisible(verot);
    // Ensimmäisenä combossa verottomuus
    ui->alvprossaLabel->setVisible(verot && ui->alvCombo->currentIndex() > 0);
    ui->alvSpin->setVisible(verot && ui->alvCombo->currentIndex() > 0 );

    if(!verot)
        ui->alvSpin->setValue(0);


    if( indeksi == MENO )
    {
        verofiltteri.setFilterRegExp("(0|.2)");
        ui->tiliEdit->suodataTyypilla("(AP|D).*");
        ui->vastatiliEdit->suodataTyypilla("[AB].*");
    }
    else if( indeksi == TULO)
    {
        verofiltteri.setFilterRegExp("(0|.1)");
        ui->tiliEdit->suodataTyypilla("(AP|C).*");
        ui->vastatiliEdit->suodataTyypilla("[AB].*");
    }
    else
    {
        ui->tiliEdit->suodataTyypilla("");
        ui->vastatiliEdit->suodataTyypilla("");
    }

    if( indeksi == MENO)
        ui->tiliLabel->setText("Menotili");
    else if(indeksi == TULO)
        ui->tiliLabel->setText("Tulotili");
    else if(indeksi == SIIRTO)
        ui->tiliLabel->setText("Debet-tili");

    if( indeksi == SIIRTO)
        ui->vastatiliLabel->setText("Kredit-tili");
    else
        ui->vastatiliLabel->setText("Vastatili");

    ehdota();
}

void KirjausApuriDialog::korjaaSarakeLeveydet()
{
    ui->ehdotusView->setColumnWidth( EhdotusModel::TILI, ui->ehdotusView->width() - ui->ehdotusView->columnWidth(EhdotusModel::KREDIT)
                                     - ui->ehdotusView->columnWidth(EhdotusModel::DEBET) - 10);
    ui->tiliEdit->setFocus();
}

void KirjausApuriDialog::accept()
{
    ehdota();
    ehdotus.tallenna(model->vientiModel());
    QDialog::accept();
}

VientiRivi KirjausApuriDialog::uusiEhdotusRivi(Tili tili, int debetSnt, int kreditSnt)
{
    VientiRivi rivi;
    rivi.pvm = ui->pvmDate->date();
    rivi.selite = ui->seliteEdit->text();
    rivi.tili = tili;
    rivi.debetSnt = debetSnt;
    rivi.kreditSnt = kreditSnt;
    return rivi;
}

void KirjausApuriDialog::laskeNetto()
{
    double brutto = ui->euroSpin->value();
    if( brutto )
    {
        bruttoEur = ui->euroSpin->value();
        nettoEur = 0;

         ui->nettoSpin->setValue( (100.0 * brutto) /  (100.0 + ui->alvSpin->value())  );
    }
    ehdota();
}
