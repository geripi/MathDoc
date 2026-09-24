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

#include "keycapobject.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QTextDocument>

KeyCapObject::KeyCapObject(QObject *parent)
: QObject(parent)
{
}

QSizeF KeyCapObject::intrinsicSize(QTextDocument *, int, const QTextFormat &format) {
    const QString text = format.property(QTextFormat::UserProperty).toString();
    
    QFont font;
    font.setPointSize(9);
    
    QFontMetricsF fm(font);
    
    const qreal horizontalPadding = 10.0;
    const qreal verticalPadding   = 4.0;
    
    return QSizeF(fm.horizontalAdvance(text) + horizontalPadding,
                  fm.height() + verticalPadding);
}

void KeyCapObject::drawObject(QPainter *painter, const QRectF &rect, QTextDocument *, int, const QTextFormat &format) {
    const QString text = format.property(QTextFormat::UserProperty).toString();
    QFont font;
    font.setPointSize(9);
    
    painter->save();
    
    painter->setFont(font);
    // Leave a little room for the border.
    const QRectF r = rect.adjusted(1, 1, -1, -1);
    // Key background
    painter->setBrush(QColor("#eeeeee"));
    // Key border
    painter->setPen(QPen(QColor("#888888"), 1));
    painter->drawRoundedRect(r, 3, 3);
    // Text
    painter->setPen(QColor("#222222"));
    painter->drawText(r, Qt::AlignCenter, text);
    
    painter->restore();
}
