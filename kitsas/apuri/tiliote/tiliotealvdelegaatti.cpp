#include "tiliotealvdelegaatti.h"

#include "tiliotealvcombo.h"
#include "tiliotekirjausrivi.h"
#include "db/kirjanpito.h"

TilioteAlvDelegaatti::TilioteAlvDelegaatti(QObject *parent)
    : QItemDelegate{parent}
{

}

QWidget *TilioteAlvDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    TilioteAlvCombo* combo = new TilioteAlvCombo(parent);
    return combo;
}

void TilioteAlvDelegaatti::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    TilioteAlvCombo* combo = qobject_cast<TilioteAlvCombo*>(editor);

    // Tilin mukaisesti ...
    const int tiliNumero = index.data(TilioteKirjausRivi::TiliRooli).toInt();
    if( tiliNumero ) {
        Tili* tili = kp()->tilit()->tili(tiliNumero);
        if(tili) {
            if( tili->onko(TiliLaji::TULO))
                combo->alustaTulolle();
            else if( tili->onko(TiliLaji::MENO))
                combo->alustaMenolle();
            combo->aseta( index.data(Qt::EditRole).toInt() );
        }
    }

}

void TilioteAlvDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    TilioteAlvCombo* combo = qobject_cast<TilioteAlvCombo*>(editor);
    model->setData(index, combo->koodi());
}
