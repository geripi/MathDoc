/* Copyright (C) 2025, 2026 Gerald Pichler (gerald.pichler@chello.at)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//pagegraphicsview.h
#ifndef PAGEGRAPHICSVIEW_H
#define PAGEGRAPHICSVIEW_H

#include <QGraphicsView>
#include "data.h"

class PageGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit PageGraphicsView(QGraphicsScene *scene, Data* d, QWidget *parent = nullptr);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    qreal m_zoomFactor = 1.0;
    void zoomBy(qreal factor);
    Data *m_data;
    
};

#endif // PAGEGRAPHICSVIEW_H
