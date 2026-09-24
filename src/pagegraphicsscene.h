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

// pagegraphicsscene.h
#ifndef PAGEGRAPHICSSCENE_H
#define PAGEGRAPHICSSCENE_H

#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QList>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QDebug>
#include <QFocusEvent>

#include "pagetextitem.h"
#include "pagemathitem.h"
#include "pagediagramitem.h"
#include "pageimageitem.h"
#include "pageA4item.h"
#include "data.h"

// Forward declaration of MainWindow
class MainWindow;

class PageGraphicsScene : public QGraphicsScene
{
    Q_OBJECT

public:
    static const qreal LINE_HEIGHT; // Declare the static const for line height
    explicit PageGraphicsScene(MainWindow *mainWindow, Data *d, QObject *parent = nullptr);

    void addA4Page();
    int pageCount() const;
    QList<PageA4Item*> getPages() { return m_pages; }
    QPointF cursorPosition() { return m_cursorItem->pos(); }
    void setCursorPosition(const QPointF &pos);
    void insertTextFrame();
    void insertMathFrame();
    void insertDiagramFrame();
    void insertImageFrame();
    void insertFrames(const QList<QGraphicsItem*> &copiedItems);
    void clearContent();
    QList<PageTextItem*> textFrames() const;
    QList<PageMathItem*> mathFrames() const;
    QList<PageDiagramItem*> diagramFrames() const;
    QList<PageImageItem*> imageFrames() const;
    PageTextItem* addTextFrame(const QString &text = QString(), const QPointF &pos = QPointF());
    PageMathItem* addMathFrame(const QPointF &pos = QPointF());
    PageDiagramItem* addDiagramFrame(const QPointF &pos = QPointF());
    PageImageItem* addImageFrame(const QPointF &pos = QPointF());
    QSizeF pageSizePx() const;
    void removeItem(QGraphicsItem *item);
    void addItem(QGraphicsItem *item);
    MainWindow *getMainWindow() { return m_MainWindow; }

signals:
    void documentContentItemsChanged();
    void modifiedChanged(bool modified);
    void cursorMoved(const QPointF &newCursorPos, qreal cursorHeight);
    
public slots:
    void compute();
    void onItemEditingStarted();
    void onItemEditingFinished();
    
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
    void keyPressEvent(QKeyEvent *keyEvent) override;
    // Override focus event handlers for the scene
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    
private:
    MainWindow *m_MainWindow;
    QList<PageA4Item*> m_pages;
    QGraphicsLineItem *m_cursorItem = nullptr;
    bool m_sceneCursorShouldBeVisible = false; // Flag to track intended visibility

    const QSizeF m_pageSizeMM = QSizeF(210.0, 297.0); // DIN A4 in mm
    const qreal m_dpi = 96.0;

    void updateSceneRect();
    void setModified(bool modified); // Helper function for modifiedChanged signal
    qreal m_cursorHeight; // Store the actual height of your cursor item
    bool itemIsPageTextItem(QGraphicsItem *item) { if(PageTextItem* textItem = dynamic_cast<PageTextItem*>(item)) return true; return false; }
    bool itemIsPageMathItem(QGraphicsItem *item) { if(PageMathItem* mathItem = dynamic_cast<PageMathItem*>(item)) return true; return false; }
//    bool itemIsMathLeafItem(QGraphicsItem *item) { if(MathLeafEdit* leafItem = dynamic_cast<MathLeafEdit*>(item)) return true; return false; }
    bool itemIsMathLeafItem(QGraphicsItem *item) { if(MathEdit* leafItem = dynamic_cast<MathEdit*>(item)) return true; return false; }
    bool itemIsQGraphicsRectItem(QGraphicsItem *item) { if(QGraphicsRectItem* textItem = dynamic_cast<QGraphicsRectItem*>(item)) return true; return false; }
    bool itemIsPageA4Item(QGraphicsItem *item) { if(PageA4Item* textItem = dynamic_cast<PageA4Item*>(item)) return true; return false; }
    Data *m_data;

private slots:
    void handleFocusChange(QGraphicsItem* newFocus, QGraphicsItem* oldFocus, Qt::FocusReason reason);
};

#endif // PAGEGRAPHICSSCENE_H
