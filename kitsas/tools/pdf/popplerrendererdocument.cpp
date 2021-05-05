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
#include "popplerrendererdocument.h"



PopplerRendererDocument::PopplerRendererDocument(const QByteArray &data)
{
    pdfDoc_ = Poppler::Document::loadFromData(data);
}

PopplerRendererDocument::~PopplerRendererDocument()
{
    if(pdfDoc_)
        delete pdfDoc_;
}

int PopplerRendererDocument::pageCount()
{
    if(pdfDoc_)
        return pdfDoc_->numPages();
    else
        return 0;
}

QImage PopplerRendererDocument::page(int page, double xres, double yres)
{
    if( !pdfDoc_ || locked())
        return QImage();

    Poppler::Page *pdfSivu = pdfDoc_->page(page);
    if( !pdfSivu)
        return QImage();

    return pdfSivu->renderToImage(xres, yres);
}

bool PopplerRendererDocument::locked() const
{
    if( pdfDoc_)
        return pdfDoc_->isLocked();
    else
        return false;
}
