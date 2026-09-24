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

#ifndef KEYCAPOBJECT_H
#define KEYCAPOBJECT_H

#include <QObject>
#include <QTextObjectInterface>
#include <QTextFormat>
#include <QSizeF>
#include <QPainter>

class KeyCapObject : public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)
    
public:
    static constexpr int Type = QTextFormat::UserObject + 1;
    
    explicit KeyCapObject(QObject *parent = nullptr);
    
    QSizeF intrinsicSize(QTextDocument *doc, int posInDocument,
                         const QTextFormat &format) override;
                         
    void drawObject(QPainter *painter, const QRectF &rect, QTextDocument *doc,
                    int posInDocument, const QTextFormat &format) override;
};

#endif // KEYCAPOBJECT_H
