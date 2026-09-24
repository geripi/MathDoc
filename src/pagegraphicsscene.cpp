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

// pagegraphicsscene.cpp
#include "pagegraphicsscene.h"
#include "mainwindow.h"

#include <QPen>
#include <QGraphicsTextItem>
#include <QTimer>
#include <QApplication> // For QApplication::keyboardModifiers()
#include <QFont>
#include <algorithm>

// Initialize the static const member for line height
const qreal PageGraphicsScene::LINE_HEIGHT = 20.0; //QFontMetricsF(QFont("Arial", 12)).lineSpacing();

PageGraphicsScene::PageGraphicsScene(MainWindow *mainWindow, Data *d, QObject *parent)
    : QGraphicsScene(parent), m_MainWindow(mainWindow), m_data(d)
{
    addA4Page(); // Add the first page
    
    m_cursorItem = addLine(0, 0, 0, 20, QPen(Qt::blue, 2, Qt::SolidLine));
    m_cursorItem->setZValue(1000); // Ensure it's on top of all items
    m_cursorItem->setVisible(false);
    m_sceneCursorShouldBeVisible = false; // Initialize the flag as false
    m_cursorHeight = m_cursorItem->line().p2().y() - m_cursorItem->line().p1().y(); // Get the cursor line's height (should be 20.0)
    
    // Connect to focus changes to manage scene cursor visibility
    connect(this, &QGraphicsScene::focusItemChanged, this, &PageGraphicsScene::handleFocusChange);
    
    // Add blinking behavior using QTimer
    QTimer *blinkTimer = new QTimer(this);
    connect(blinkTimer, &QTimer::timeout, this, [=]() {
        // Only blink if the cursor is *intended* to be visible
        if (m_sceneCursorShouldBeVisible) {
            m_cursorItem->setVisible(!m_cursorItem->isVisible()); // Toggle visibility
        } else {
            // Ensure the cursor is hidden if it's not supposed to be visible
            m_cursorItem->setVisible(false);
        }
    });
    blinkTimer->start(500); // Blink every 500ms
}

QSizeF PageGraphicsScene::pageSizePx() const
{
    return QSizeF(
        m_pageSizeMM.width() * m_dpi / 25.4,
        m_pageSizeMM.height() * m_dpi / 25.4
    );
}

void PageGraphicsScene::addA4Page()
{
    QSizeF size = pageSizePx();
    qreal yOffset = m_pages.isEmpty() ? 0 : m_pages.last()->rect().bottom() + 0; // 0px gap between pages

    auto *rect = new PageA4Item(0, yOffset, size.width(), size.height());
    rect->setPen(QPen(Qt::black));
    rect->setBrush(Qt::white);
    rect->setZValue(-1); // Draw pages behind other content
    addItem(rect);
    m_pages.append(rect);

    updateSceneRect(); // Update scene rect after adding a page
    //qDebug() << "PageGraphicsScene: Added new page at y =" << yOffset;
}

int PageGraphicsScene::pageCount() const
{
    return m_pages.count();
}

void PageGraphicsScene::setCursorPosition(const QPointF &pos)
{
    bool cursorOnPage = false; // Flag to track if cursor is on any page
    for (auto *page : m_pages) {
        // Check if the position is within this page's boundaries
        // We adjust pos by page->pos() because page->contains expects coordinates relative to the item itself.
        if (page->contains(pos - page->pos())) {
            // Adjust cursor position to be within the page boundaries if it's too close to the edge
            QPointF adjustedPos = pos;
            if (adjustedPos.x() < page->rect().left() + 5) adjustedPos.setX(page->rect().left() + 5);
            if (adjustedPos.x() > page->rect().right() - 5) adjustedPos.setX(page->rect().right() - 5);
            if (adjustedPos.y() < page->rect().top() + 5) adjustedPos.setY(page->rect().top() + 5);
            if (adjustedPos.y() > page->rect().bottom() - 5) adjustedPos.setY(page->rect().bottom() - 5);
            
            // Constrain cursor height visually
            const qreal cursorHeight = 20.0; // Height of the blinking cursor line
            m_cursorItem->setLine(0.0, 0.0,
                                  0.0, cursorHeight);
            m_cursorItem->setPos(adjustedPos.x(), adjustedPos.y());
            
            m_cursorItem->setZValue(1000); // Draw on top
            // m_sceneCursorShouldBeVisible = true; // Handled by focusInEvent/focusOutEvent
            // m_cursorItem->setVisible(true);      // Handled by focusInEvent/focusOutEvent
            //qDebug() << "PageGraphicsScene: Cursor set to" << adjustedPos;
            cursorOnPage = true;
            break; // Found the page, no need to check others
        }
    }
    
    if (!cursorOnPage) {
        m_sceneCursorShouldBeVisible = false; // Intend to hide
        m_cursorItem->setVisible(false);     // Hide immediately
        //qDebug() << "PageGraphicsScene: Cursor hidden (outside any page bounds)";
    } else {
        // If cursor is on page, and scene has focus, it should be visible
        if (this->focusItem() == nullptr) { // If no item has focus, but scene can receive input
            m_sceneCursorShouldBeVisible = true;
            m_cursorItem->setVisible(true); // Show immediately when position is set
        }
    }
}

// Override addItem to connect to new PageTextItem's signals
void PageGraphicsScene::addItem(QGraphicsItem *item) {
    QGraphicsScene::addItem(item); // Call base class implementation first
    if (PageTextItem *textItem = qgraphicsitem_cast<PageTextItem*>(item)) {
        connect(textItem, &PageTextItem::itemDataChanged,
                this, &PageGraphicsScene::documentContentItemsChanged);
        // Connect the new item's editing signals to scene slots
        connect(textItem, &PageTextItem::editingStarted, this, &PageGraphicsScene::onItemEditingStarted);
        connect(textItem, &PageTextItem::editingFinished, this, &PageGraphicsScene::onItemEditingFinished);
        connect(textItem, &PageTextItem::itemSelected, m_MainWindow, &MainWindow::updateUi);
    }
    if (PageMathItem *mathItem = qgraphicsitem_cast<PageMathItem*>(item)) {
        connect(mathItem, &PageMathItem::itemDataChanged,
                this, &PageGraphicsScene::documentContentItemsChanged);
        // Connect the new item's editing signals to scene slots
        connect(mathItem, &PageMathItem::editingStarted, this, &PageGraphicsScene::onItemEditingStarted);
        connect(mathItem, &PageMathItem::editingFinished, this, &PageGraphicsScene::onItemEditingFinished);
        connect(mathItem, &PageMathItem::itemSelected, m_MainWindow, &MainWindow::updateUi);
        connect(mathItem, &PageMathItem::computeWholeDocument, this, &PageGraphicsScene::compute);
    }
    if (PageDiagramItem *diagramItem = qgraphicsitem_cast<PageDiagramItem*>(item)) {
        connect(diagramItem, &PageDiagramItem::itemDataChanged,
                this, &PageGraphicsScene::documentContentItemsChanged);
        // Connect the new item's editing signals to scene slots
        connect(diagramItem, &PageDiagramItem::itemSelected, m_MainWindow, &MainWindow::updateUi);
    }
    if (PageImageItem *imageItem = qgraphicsitem_cast<PageImageItem*>(item)) {
        connect(imageItem, &PageImageItem::itemDataChanged,
                this, &PageGraphicsScene::documentContentItemsChanged);
        // Connect the new item's editing signals to scene slots
        connect(imageItem, &PageImageItem::itemSelected, m_MainWindow, &MainWindow::updateUi);
    }
    emit documentContentItemsChanged(); // A new item was added, so content changed
}

// Override removeItem to disconnect from removed PageTextItem's signals
void PageGraphicsScene::removeItem(QGraphicsItem *item) {
    if (PageTextItem *textItem = qgraphicsitem_cast<PageTextItem*>(item)) {
        disconnect(textItem, &PageTextItem::itemDataChanged,
                   this, &PageGraphicsScene::documentContentItemsChanged);
        disconnect(textItem, &PageTextItem::editingStarted, this, &PageGraphicsScene::onItemEditingStarted);
        disconnect(textItem, &PageTextItem::editingFinished, this, &PageGraphicsScene::onItemEditingFinished);
        disconnect(textItem, &PageTextItem::itemSelected, m_MainWindow, &MainWindow::updateUi);
        emit documentContentItemsChanged(); // Item removed, so content changed
    }
    if (PageMathItem *mathItem = qgraphicsitem_cast<PageMathItem*>(item)) {
        disconnect(mathItem, &PageMathItem::itemDataChanged,
                   this, &PageGraphicsScene::documentContentItemsChanged);
        disconnect(mathItem, &PageMathItem::editingStarted, this, &PageGraphicsScene::onItemEditingStarted);
        disconnect(mathItem, &PageMathItem::editingFinished, this, &PageGraphicsScene::onItemEditingFinished);
        disconnect(mathItem, &PageMathItem::itemSelected, m_MainWindow, &MainWindow::updateUi);
        disconnect(mathItem, &PageMathItem::computeWholeDocument, this, &PageGraphicsScene::compute);
        emit documentContentItemsChanged(); // Item removed, so content changed
    }
    if (PageDiagramItem *DiagramItem = qgraphicsitem_cast<PageDiagramItem*>(item)) {
        disconnect(DiagramItem, &PageDiagramItem::itemDataChanged,
                   this, &PageGraphicsScene::documentContentItemsChanged);
        disconnect(DiagramItem, &PageDiagramItem::itemSelected, m_MainWindow, &MainWindow::updateUi);
        emit documentContentItemsChanged(); // Item removed, so content changed
    }
    QGraphicsScene::removeItem(item); // Call base class implementation last
}


void PageGraphicsScene::insertTextFrame()
{
    // Check if cursor is on a page AND the scene currently has focus
    if (!m_cursorItem || !m_sceneCursorShouldBeVisible) {
        //qDebug() << "PageGraphicsScene: Cannot insert text frame, scene cursor not active.";
        return;
    }
    PageTextItem *item = new PageTextItem(); // Calls PageTextItem(QGraphicsItem *parent = nullptr)
    
    item->setPos(m_cursorItem->pos());
    addItem(item);
    item->setTextInteractionFlags(Qt::TextEditorInteraction); // Make it immediately editable
    item->setFocus(); // Give focus to allow immediate typing
    emit item->itemSelected(); // Tell everybody that the item is now selected
    // When a new text frame is inserted and gains focus, the scene cursor should hide.
    m_sceneCursorShouldBeVisible = false; // Scene cursor hides when item gets focus
    m_cursorItem->setVisible(false);
    //qDebug() << "PageGraphicsScene::insertTextFrame: Inserted new text frame at" << m_cursorItem->pos();
}

void PageGraphicsScene::insertMathFrame()
{
    // Check if cursor is on a page AND the scene currently has focus
    if (!m_cursorItem || !m_sceneCursorShouldBeVisible) {
        qDebug() << "PageGraphicsScene: Cannot insert math frame, scene cursor not active.";
        return;
    }
    PageMathItem *item = new PageMathItem(m_data); // Calls PageMathItem(QGraphicsItem *parent = nullptr)
    
    item->setPos(m_cursorItem->pos());
    addItem(item);
    m_sceneCursorShouldBeVisible = false; // Scene cursor hides when item gets focus
    m_cursorItem->setVisible(false);
    emit item->itemSelected(); // Tell everybody that the item is now selected
    // When a new math frame is inserted and gains focus, the scene cursor should hide.
    qDebug() << "PageGraphicsScene::insertMathFrame: Inserted new math frame at" << m_cursorItem->pos();
}

void PageGraphicsScene::insertDiagramFrame()
{
    // Check if cursor is on a page AND the scene currently has focus
    if (!m_cursorItem || !m_sceneCursorShouldBeVisible) {
        qDebug() << "PageGraphicsScene: Cannot insert diagram frame, scene cursor not active.";
        return;
    }
    PageDiagramItem *item = new PageDiagramItem(m_data); // Calls PageDiagramItem(QGraphicsItem *parent = nullptr)
    item->initTitles();
    item->setPos(m_cursorItem->pos());
    
    addItem(item);
    m_sceneCursorShouldBeVisible = false; // Scene cursor hides when item gets focus
    m_cursorItem->setVisible(false);
    emit item->itemSelected(); // Tell everybody that the item is now selected
    // When a new text frame is inserted and gains focus, the scene cursor should hide.
    qDebug() << "PageGraphicsScene::insertdiagramFrame: Inserted new diagram frame at" << m_cursorItem->pos();
}

void PageGraphicsScene::insertImageFrame()
{
    // Check if cursor is on a page AND the scene currently has focus
    if (!m_cursorItem || !m_sceneCursorShouldBeVisible) {
        qDebug() << "PageGraphicsScene: Cannot insert diagram frame, scene cursor not active.";
        return;
    }
    PageImageItem *item = PageImageItem::createFromFileDialog(nullptr);
    item->setPos(m_cursorItem->pos());
    
    addItem(item);
    m_sceneCursorShouldBeVisible = false; // Scene cursor hides when item gets focus
    m_cursorItem->setVisible(false);
    // When a new text frame is inserted and gains focus, the scene cursor should hide.
    qDebug() << "PageGraphicsScene::insertdiagramFrame: Inserted new diagram frame at" << m_cursorItem->pos();
}

// Clears all content and adds one initial page.
void PageGraphicsScene::clearContent() {
    // Remove all items from the scene except the cursor
    QList<QGraphicsItem*> allItems = items();
    for (QGraphicsItem *item : allItems) {
        if (item != m_cursorItem) { // Don't remove the cursor item
            removeItem(item);
            delete item;
        }
    }
    m_pages.clear(); // Clear the list of page rectangles
    addA4Page(); // Add one initial page
    updateSceneRect(); // Reset scene rect
    m_sceneCursorShouldBeVisible = false; // On clear, hide cursor
    m_cursorItem->setVisible(false);
    //qDebug() << "PageGraphicsScene: Cleared content and added initial page.";
}

// Returns a list of all PageTextItem objects in the scene.
QList<PageTextItem*> PageGraphicsScene::textFrames() const {
    QList<PageTextItem*> frames;
    for (QGraphicsItem *item : items()) {
        if (PageTextItem *textItem = qgraphicsitem_cast<PageTextItem*>(item)) {
            frames.append(textItem);
        }
    }
    return frames;
}

// Returns a sorted list of all PageMathItem objects in the scene.
QList<PageMathItem*> PageGraphicsScene::mathFrames() const {
    QList<PageMathItem*> frames;
    for (QGraphicsItem *item : items()) {
        if (PageMathItem *mathItem = qgraphicsitem_cast<PageMathItem*>(item)) {
            frames.append(mathItem);
        }
    }
    // sort by position
    std::sort(frames.begin(), frames.end(),
              [](QGraphicsObject *a, QGraphicsObject *b) {
                  QPointF pa = a->pos();
                  QPointF pb = b->pos();
                  if (pa.y() < pb.y()) return true;
                  if (pa.y() > pb.y()) return false;
                  return pa.x() < pb.x();  // tie-breaker on x
              });
    
    return frames;
}

// Returns a list of all PageDiagtramItem objects in the scene.
QList<PageDiagramItem*> PageGraphicsScene::diagramFrames() const {
    QList<PageDiagramItem*> frames;
    for (QGraphicsItem *item : items()) {
        if (PageDiagramItem *diagramItem = qgraphicsitem_cast<PageDiagramItem*>(item)) {
            frames.append(diagramItem);
        }
    }
    return frames;
}

// Returns a list of all PageImageItem objects in the scene.
QList<PageImageItem*> PageGraphicsScene::imageFrames() const {
    QList<PageImageItem*> frames;
    for (QGraphicsItem *item : items()) {
        if (PageImageItem *imageItem = qgraphicsitem_cast<PageImageItem*>(item)) {
            frames.append(imageItem);
        }
    }
    return frames;
}

// Implementation of addTextFrame for external use (e.g., loading from file)
PageTextItem* PageGraphicsScene::addTextFrame(const QString &text, const QPointF &pos) {
    PageTextItem *textItem = new PageTextItem();
    textItem->setPlainText(text);
    textItem->setPos(pos);
    addItem(textItem);
    qDebug() << "PageGraphicsScene: Added text frame via public method at" << pos;
    // The connections for editing signals are now handled by the overridden addItem
    clearSelection();
    return textItem;
}

// Implementation of addMathFrame for external use (e.g., loading from file)
PageMathItem* PageGraphicsScene::addMathFrame(const QPointF &pos) {
    PageMathItem *mathItem = new PageMathItem(m_data);
    mathItem->setPos(pos);
    addItem(mathItem);
    qDebug() << "PageGraphicsScene::addMathFrame: Added math frame via public method at" << pos;
    // The connections for editing signals are now handled by the overridden addItem
    clearSelection();
    return mathItem;
}

// Implementation of addDiagramFrame for external use (e.g., loading from file)
PageDiagramItem* PageGraphicsScene::addDiagramFrame(const QPointF &pos) {
    PageDiagramItem *diagramItem = new PageDiagramItem(m_data);
    diagramItem->setPos(pos);
    addItem(diagramItem);
    qDebug() << "PageGraphicsScene::addDiagramFrame: Added diagram frame via public method at" << pos;
    // The connections for editing signals are now handled by the overridden addItem
    clearSelection();
    return diagramItem;
}

// Implementation of addImageFrame for external use (e.g., loading from file)
PageImageItem* PageGraphicsScene::addImageFrame(const QPointF &pos) {
    PageImageItem *imageItem = new PageImageItem(nullptr);
    imageItem->setPos(pos);
    addItem(imageItem);
    qDebug() << "PageGraphicsScene::addImageFrame: Added image frame via public method at" << pos;
    // The connections for editing signals are now handled by the overridden addItem
    clearSelection();
    return imageItem;
}

// Helper for setting/unsetting document modification status
void PageGraphicsScene::setModified(bool modified) {
    // This function will be called by DocumentWidget once it connects to
    // PageGraphicsScene::documentContentItemsChanged.
    // So, this empty implementation is fine for now as it's a private helper.
    // The main connection that updates the document's modified state is in DocumentWidget.
    emit modifiedChanged(modified);
}

// Helper to calculate the scene rectangle based on pages.
void PageGraphicsScene::updateSceneRect() {
    if (m_pages.isEmpty()) {
        // If no pages, set a default scene rect based on the initial page size
        setSceneRect(0, 0, pageSizePx().width(), pageSizePx().height());
        //qDebug() << "PageGraphicsScene::updateSceneRect: Updated scene rect to default (no pages).";
        return;
    }

    // Calculate the bounding rectangle that encompasses all pages
    QRectF bounds = m_pages.first()->sceneBoundingRect();
    for (int i = 1; i < m_pages.size(); ++i) {
        bounds = bounds.united(m_pages.at(i)->sceneBoundingRect());
    }

    // Add some padding around the entire document area
    qreal padding = 50;
    setSceneRect(bounds.adjusted(-padding, -padding, padding, padding));
    //qDebug() << "PageGraphicsScene: Updated scene rect to" << sceneRect();
}

// Override mousePressEvent to handle clicks for adding new text frames.
void PageGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent) {
    QGraphicsScene::mousePressEvent(mouseEvent); // Pass to base class for other interactions (e.g., item selection)
}

// Override keyPressEvent to handle key presses for adding new text frames.
void PageGraphicsScene::keyPressEvent(QKeyEvent *keyEvent) {
    
    const bool altPressed = keyEvent->modifiers() & Qt::AltModifier;
    const bool shiftPressed = keyEvent->modifiers() & Qt::ShiftModifier;
    const bool ctrlPressed = keyEvent->modifiers() & Qt::ControlModifier;
    
    // If 'T' key is pressed and no item has focus, add a new text frame
    if (keyEvent->key() == Qt::Key_T && (keyEvent->modifiers() & Qt::ControlModifier) && !focusItem()) {
        insertTextFrame(); // The scene cursor hiding is handled by insertTextFrame
        keyEvent->accept();
        return;
    }
    if (keyEvent->key() == Qt::Key_M && (keyEvent->modifiers() & Qt::ControlModifier) && !focusItem()) {
        insertMathFrame(); // The scene cursor hiding is handled by insertMathFrame now
        keyEvent->accept();
        return;
    }
    if (keyEvent->key() == Qt::Key_D && (keyEvent->modifiers() & Qt::ControlModifier) && !focusItem()) {
        insertDiagramFrame(); // The scene cursor hiding is handled by insertMathFrame now
        keyEvent->accept();
        return;
    }
    if (keyEvent->key() == Qt::Key_I && (keyEvent->modifiers() & Qt::ControlModifier) && !focusItem()) {
        insertImageFrame(); // The scene cursor hiding is handled by insertMathFrame now
        keyEvent->accept();
        return;
    }
    
    if (keyEvent->key() == Qt::Key_Delete && !focusItem()) {
        if (shiftPressed) {
            keyEvent->accept();
        } else {
            QList<QGraphicsItem*> selected = selectedItems();
            for (QGraphicsItem* item : selected) {
                // Safely cast to PageTextItem or other deletable types
                if (PageTextItem* textItem = dynamic_cast<PageTextItem*>(item)) {
                    removeItem(textItem); // Remove from scene
                    delete textItem;       // Delete the item object
                    emit documentContentItemsChanged(); // to notify DocumentWidget that content has changed.
                }
                if (PageMathItem* mathItem = dynamic_cast<PageMathItem*>(item)) {
                    removeItem(mathItem); // Remove from scene
                    delete mathItem;       // Delete the item object
                    emit documentContentItemsChanged(); // to notify DocumentWidget that content has changed.
                }
                if (PageDiagramItem* diagramItem = dynamic_cast<PageDiagramItem*>(item)) {
                    removeItem(diagramItem); // Remove from scene
                    delete diagramItem;      // Delete the item object
                    emit documentContentItemsChanged(); // to notify DocumentWidget that content has changed.
                }
                // Add similar checks for other custom item types you might want to delete
            }
            keyEvent->accept(); // Mark the event as handled
        }
    }
    
    // Only handle key events if the scene cursor is intended to be visible
    // (i.e., scene has focus and no text item is being edited).
    if (m_cursorItem && m_sceneCursorShouldBeVisible) {
        QPointF currentCursorPos = m_cursorItem->pos();
        QPointF newCursorPos = currentCursorPos;
        QSizeF pageSize = pageSizePx(); // Get page size in pixels
        qDebug() << "PageGraphicsScene::keyPressEvent: currentCursorPos = " << currentCursorPos;
        // Calculate current scene boundaries based on existing pages
        qreal sceneTop = 0; // Top of the first page
        qreal sceneBottom = pageCount() * pageSize.height(); // Bottom of the last page
        qreal pageLeft = 0; // Left edge of the page (assuming content starts at X=0)
        qreal pageRight = pageSize.width(); // Right edge of the page
        
        if (keyEvent->key() == Qt::Key_Up) {
                newCursorPos.setY(currentCursorPos.y() - LINE_HEIGHT);
                // Clamp to top of first page (Y=0)
                newCursorPos.setY(qMax(sceneTop, newCursorPos.y()));
                setCursorPosition(newCursorPos);
                emit cursorMoved(newCursorPos, m_cursorHeight);
                keyEvent->accept(); // Mark event as handled.
                return; // Return after base call
        }
        else if (keyEvent->key() == Qt::Key_Down) {
                newCursorPos.setY(currentCursorPos.y() + LINE_HEIGHT);
                // Clamp to bottom of last page (accounting for cursor height)
                newCursorPos.setY(qMin(sceneBottom - m_cursorHeight, newCursorPos.y()));
                setCursorPosition(newCursorPos);
                emit cursorMoved(newCursorPos, m_cursorHeight);
                keyEvent->accept(); // Mark event as handled.
                return; // Return after base call
        }
        else if (keyEvent->key() == Qt::Key_Left) {
                newCursorPos.setX(currentCursorPos.x() - LINE_HEIGHT); // Move horizontally by line height
                // Clamp to left edge of current page
                newCursorPos.setX(qMax(pageLeft, newCursorPos.x()));
                setCursorPosition(newCursorPos);
                emit cursorMoved(newCursorPos, m_cursorHeight);
                keyEvent->accept(); // Mark event as handled.
                return; // Return after base call
        }
        else if (keyEvent->key() == Qt::Key_Right) {
                newCursorPos.setX(currentCursorPos.x() + LINE_HEIGHT); // Move horizontally by line height
                // Clamp to right edge of current page
                newCursorPos.setX(qMin(pageRight, newCursorPos.x()));
                setCursorPosition(newCursorPos);
                emit cursorMoved(newCursorPos, m_cursorHeight);
                keyEvent->accept(); // Mark event as handled.
                return; // Return after base call
        }
        else if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {  // Handles Numpad Enter key and normal Return key
            // Move cursor down by LINE_HEIGHT
            newCursorPos.setY(currentCursorPos.y() + LINE_HEIGHT);
            
            qreal lowestItemBottom = 0; // Track the lowest point of any item after potential moves
            // Iterate through all text items and shift those below the cursor
            for (QGraphicsItem* item : items()) { // Assuming you have or will add textFrames() helper
                // Check if the item's top is below or at the original cursor Y position.
                // A small tolerance (e.g., -1) might be needed for floating point precision.
                if(!itemIsPageA4Item(item) && !itemIsMathLeafItem(item)) {
                    qDebug() << "PageGraphicsScene::keyPressEvent: downshifting item " << item;
                    if (item->sceneBoundingRect().top() >= currentCursorPos.y() - 1) {
                        item->setPos(item->pos().x(), item->pos().y() + LINE_HEIGHT);
                    }
                    // Update the lowest item's bottom after its potential move
                    if (item->sceneBoundingRect().bottom() > lowestItemBottom && !itemIsPageA4Item(item)) {
                        lowestItemBottom = item->sceneBoundingRect().bottom();
                    }
                }
            }
            
            // Determine the effective lowest point that needs to be contained within pages.
            // This is the maximum of the lowest shifted item's bottom or the new cursor position's bottom.
            qreal effectiveLowestY = qMax(lowestItemBottom, m_cursorItem->pos().y() + m_cursorHeight);
            
            // Add new pages if required to accommodate the new lowest position
            int currentPageCount = pageCount();
            int requiredPages = qCeil(effectiveLowestY / pageSize.height());
            
            // Ensure at least one page is present if content exists but doesn't fill a full page
            if (requiredPages == 0 && effectiveLowestY > 0) {
                requiredPages = 1;
            } else if (effectiveLowestY <= 0) { // No items, or all moved up out of view
                requiredPages = currentPageCount; // Don't reduce page count
            }
            
            while (currentPageCount < requiredPages) {
                addA4Page(); // Call your scene's method to add a new page.
                currentPageCount++; // Increment count to reflect the new page.
                qDebug() << "PageGraphicsScene: Added new page due to RETURN key. Total pages:" << currentPageCount;
            }
            setCursorPosition(newCursorPos);
            // Update the scene's bounding rectangle to reflect moved items and new pages.
            updateSceneRect();
            // Emit signal to notify MainWindow that document content has changed (for modified flag).
            emit documentContentItemsChanged();
            emit cursorMoved(newCursorPos, m_cursorHeight);
            keyEvent->accept(); // Mark event as handled.
            return;
        }
        else {
            // For other keys, pass to the base class's keyPressEvent for default behavior.
            QGraphicsScene::keyPressEvent(keyEvent);
            return; // Return after base call
        }
        // Apply new cursor position for arrow keys if the event was an arrow key
//        keyEvent->accept(); // Mark arrow key event as handled
//        return;
    }
    
    
    QGraphicsScene::keyPressEvent(keyEvent); // Pass to base class for other interactions
}

void PageGraphicsScene::insertFrames(const QList<QGraphicsItem*> &copiedItems) {
    QList<QGraphicsItem*> newItems;
    
    qreal minX = std::numeric_limits<qreal>::max(),
          minY = std::numeric_limits<qreal>::max();
          
    for (QGraphicsItem *item : copiedItems) {
        if (!item) continue;
        if (item->pos().x()<minX) minX = item->pos().x();
        if (item->pos().y()<minY) minY = item->pos().y();
    }
    
    qreal xShift = m_cursorItem->pos().x() - minX;
    qreal yShift = m_cursorItem->pos().y() - minY;
    
    for (QGraphicsItem *item : copiedItems) {
        if (!item) continue;
        
        const int type = item->type();
        
        if (type == PageTextItem::Type) {
            PageTextItem *original = qgraphicsitem_cast<PageTextItem *>(item);
            if (!original) continue;
            QJsonObject object = original->toJson();
            PageTextItem *copy = PageTextItem::fromJson(object);
            if (!copy) continue;
            addItem(copy);
            copy->setPos(copy->pos().x() + xShift, copy->pos().y() + yShift);
            newItems.append(copy);
        } else if (type == PageMathItem::Type) {
            PageMathItem *original = qgraphicsitem_cast<PageMathItem *>(item);
            if (!original) continue;
            if (original->getIsInDiagramTitle()) continue; // units in diagram titles are taken care of from within json
            QJsonObject object = original->toJson();
            PageMathItem *copy = PageMathItem::fromJson(object, m_data);
            if (!copy) continue;
            addItem(copy);
            copy->setPos(copy->pos().x() + xShift, copy->pos().y() + yShift);
            copy->updateLayout();
            newItems.append(copy);
        } else if (type == PageDiagramItem::Type) {
            PageDiagramItem *original = qgraphicsitem_cast<PageDiagramItem *>(item);
            if (!original) continue;
            QJsonObject object = original->toJson();
            PageDiagramItem *copy = PageDiagramItem::fromJson(object, m_data);
            if (!copy) continue;
            addItem(copy);
            copy->setPos(copy->pos().x() + xShift, copy->pos().y() + yShift);
            newItems.append(copy);
        }
        else if (type == PageImageItem::Type)
        {
            PageImageItem *original = qgraphicsitem_cast<PageImageItem *>(item);
            if (!original) continue;
            QJsonObject object = original->toJson();
            PageImageItem *copy = new PageImageItem();
            if (!copy->fromJson(object)) {
                delete copy;
                continue;
            }
            addItem(copy);
            copy->setPos(copy->pos().x() + xShift, copy->pos().y() + yShift);
            newItems.append(copy);
        }
    }
    
    // Replace selection with the newly created copies.
    clearSelection();
    /*
    for (QGraphicsItem *item : newItems) {
        item->setSelected(true);
    }
    */
}

// Handle scene gaining focus
void PageGraphicsScene::focusInEvent(QFocusEvent *event) {
    QGraphicsScene::focusInEvent(event); // Call base class
    // If no item specifically has focus, then the scene itself has focus for input
    if (this->focusItem() == nullptr) {
        m_sceneCursorShouldBeVisible = true;
        m_cursorItem->setVisible(true); // Make it immediately visible
        //qDebug() << "PageGraphicsScene::focusInEvent: Scene gained focus, cursor set to be visible.";
        //compute();
    }
}

// Handle scene losing focus
void PageGraphicsScene::focusOutEvent(QFocusEvent *event) {
    QGraphicsScene::focusOutEvent(event); // Call base class
    m_sceneCursorShouldBeVisible = false;
    m_cursorItem->setVisible(false); // Hide immediately
    //qDebug() << "PageGraphicsScene::focusOutEvent: Scene lost focus, cursor hidden.";
}

// Slot to hide the scene cursor when a text item starts editing
void PageGraphicsScene::onItemEditingStarted() {
    if (m_cursorItem) {
        m_sceneCursorShouldBeVisible = false; // Intend to hide
        m_cursorItem->setVisible(false);      // Hide immediately
        qDebug() << "PageGraphicsScene::onItemEditingStarted: Hidden scene cursor (text editing started).";
    }
}

// Slot to show the scene cursor when a text item finishes editing
void PageGraphicsScene::onItemEditingFinished() {
    if (m_cursorItem) {
        // Only show if no other item is currently editing.
        bool anotherItemIsEditing = false;
        for (QGraphicsItem* item : items()) {
            if (PageTextItem* textItem = dynamic_cast<PageTextItem*>(item)) {
                if (textItem->textInteractionFlags() == Qt::TextEditorInteraction) {
                    anotherItemIsEditing = true;
                    break;
                }
            }
            if (PageMathItem* mathItem = dynamic_cast<PageMathItem*>(item)) {
                if (mathItem->isEditing()) {
                    anotherItemIsEditing = true;
                    break;
                }
            }
        }

        // Only make the scene cursor visible if no other item is editing AND the scene still has focus
        if (!anotherItemIsEditing && this->focusItem() == nullptr) {
            m_sceneCursorShouldBeVisible = true; // Intend to show
            m_cursorItem->setVisible(true); // Make it immediately visible
            // The timer will handle blinking it on/off.
            qDebug() << "PageGraphicsScene::onItemEditingFinished: Scene cursor set to be visible (text editing finished, scene focus).";
        } else {
            // If another item is editing OR scene lost focus, ensure the scene cursor remains hidden
            m_sceneCursorShouldBeVisible = false;
            m_cursorItem->setVisible(false);
            qDebug() << "PageGraphicsScene::onItemEditingFinished: Another item is editing OR scene lost focus, scene cursor remains hidden.";
        }
    }
}

void PageGraphicsScene::handleFocusChange(QGraphicsItem* newFocus, QGraphicsItem* oldFocus, Qt::FocusReason reason) {
    Q_UNUSED(oldFocus);
    Q_UNUSED(reason);
    
    if (newFocus) {
        // An item has gained focus, so hide the scene cursor
        m_sceneCursorShouldBeVisible = false;
        m_cursorItem->setVisible(false);
    } else {
        // No item has focus, so show the scene cursor
        m_sceneCursorShouldBeVisible = true;
        m_cursorItem->setVisible(true);
        // Compute the document
        compute();
    }
}

void PageGraphicsScene::compute() {
qDebug() << "PageGraphicsScene::compute: Starting computation of whole document.";
    m_data->clear(); // delete all previous calculations
    QList<PageMathItem*> temporaryNotComputed;
    for (PageMathItem* item: mathFrames()) {
        for(int32_t i = temporaryNotComputed.length()-1; i >=0; i--) {
            PageMathItem* retryItem = temporaryNotComputed.at(i);
            Data *tmpData = new Data();
            tmpData->deepCopy(m_data); // cash the m_data if the item computation fails
            
            retryItem->compute();
            if (!retryItem->hasErrorMessages()) {
                temporaryNotComputed.removeAt(i);
            } else {
                m_data->deepCopy(tmpData); // reset the m_data to the cashed state
            }
            delete tmpData;
        }
        
        Data *tmpData = new Data();
        tmpData->deepCopy(m_data); // cash the m_data if the item computation fails
        
        item->compute();
        
        if (item->hasErrorMessages()) {
            temporaryNotComputed.append(item);
            m_data->deepCopy(tmpData); // reset the m_data to the cashed state
        }
        delete tmpData;
    }
    for(int32_t i = temporaryNotComputed.length()-1; i >=0; i--) {
        PageMathItem* retryItem = temporaryNotComputed.at(i);
        retryItem->compute();
        if (!retryItem->hasErrorMessages()) {
            temporaryNotComputed.removeAt(i);
        }
    }
    
    
    for (PageDiagramItem* item: diagramFrames()) {
        item->refresh();
    }
}
