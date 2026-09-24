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

// documentwidget.cpp
#include "documentwidget.h"
#include "mainwindow.h"

#include <QVBoxLayout> // For arranging widgets vertically
#include <QDebug>
#include <QJsonParseError> // For JSON parsing errors
#include <QTextStream> // For writing text to file
#include <QFile>       // For file operations
#include <QPdfWriter>
#include <QPainter>
#include <QPageSize>
#include <QPageLayout>
#include <QFileInfo>

// Constructor for DocumentWidget.
DocumentWidget::DocumentWidget(MainWindow *mainWindow, QWidget *parent, const QString &filePath)
    : QWidget(parent), m_MainWindow(mainWindow), m_filePath(filePath), m_isModified(false) { // Initialize m_isModified
    // Create a new PageGraphicsScene and PageGraphicsView.
    m_data = new Data(this);
    m_graphicsScene = new PageGraphicsScene(m_MainWindow, m_data, this); // Scene is created here
    m_graphicsView = new PageGraphicsView(m_graphicsScene, m_data, this); // View is created here

    // Set the layout for the widget. QVBoxLayout arranges widgets in a column.
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_graphicsView); // Add the graphics view to the layout
    layout->setContentsMargins(0, 0, 0, 0); // Remove margins around the view

    // If a file path is provided, try to load the document.
    if (!m_filePath.isEmpty()) {
        load();
    } else {
        // For a new, unsaved document, clear content (which adds one page) and set modified to false.
        m_graphicsScene->clearContent(); // Now calls the implemented clearContent
        setModified(false); // Explicitly set modified to false
    }
    // Connect to the specific signal from PageGraphicsScene when its content items change
    connect(m_graphicsScene, &PageGraphicsScene::documentContentItemsChanged,
        this, &DocumentWidget::onDocumentContentItemsChanged); // Rename onSceneChanged to something more specific
    // Connect the cursorMoved signal from the scene to a slot in DocumentWidget
    connect(m_graphicsScene, &PageGraphicsScene::cursorMoved,
        this, &DocumentWidget::onCursorMoved);
    qDebug() << "DocumentWidget constructor:" << this << "Path:" << m_filePath << "Modified:" << isModified();
}

// Returns true if the document has been modified since the last save.
bool DocumentWidget::isModified() const {
    return m_isModified; // Return the internal flag
}

// Sets the file path for the document.
void DocumentWidget::setFilePath(const QString &path) {
    if (m_filePath != path) { // Only update if path is different
        m_filePath = path;
        // After setting a new path, the document is considered "saved" to that path
        // until further modifications.
        setModified(false); // Clear the modified flag
        qDebug() << "DocumentWidget setFilePath:" << this << "New Path:" << m_filePath << "Modified:" << isModified();
    }
}

// Clears the modified flag of the document.
void DocumentWidget::setModified(bool modified) {
    if (m_isModified != modified) { // Only emit if state actually changes
        m_isModified = modified; // Set the internal flag
        // Emit the contentModified signal to notify parent widgets (e.g., MdiTabbedDocumentArea)
        // so they can update the tab title (add/remove '*').
        emit contentModified();
        qDebug() << "DocumentWidget setModified:" << this << "to" << modified;
    }
}

// Saves the document to its current file path.
bool DocumentWidget::save() {
    if (m_filePath.isEmpty()) {
        qDebug() << "DocumentWidget save: No file path set for" << this;
        return false; // This case should be handled by a "Save As" dialog in the main window.
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox::warning(this, "MathDoc",
                             QString("Could not write to file %1:\n%2")
                                 .arg(m_filePath, file.errorString()));
        qDebug() << "DocumentWidget save: Failed to open file" << m_filePath << ":" << file.errorString();
        return false;
    }

    // Set the guard flag BEFORE starting the save logic
    m_isSaving = true;
    qDebug() << "DocumentWidget save: Setting m_isSaving to true.";
    
    // Serialize document content to JSON
    QJsonObject docObject;
    QJsonArray framesArray;
    // Iterate through all PageTextItems in the scene and serialize their properties
    for (PageTextItem* frame : m_graphicsScene->textFrames()) {
        framesArray.append(frame->toJson());
    }
    for (PageMathItem* frame : m_graphicsScene->mathFrames()) {
        if (!frame->getIsInDiagramTitle()) {
            framesArray.append(frame->toJson());
        }
    }
    for (PageDiagramItem* frame : m_graphicsScene->diagramFrames()) {
        framesArray.append(frame->toJson());
    }
    for (PageImageItem* frame : m_graphicsScene->imageFrames()) {
        framesArray.append(frame->toJson());
    }
    docObject["frames"] = framesArray;

    QJsonDocument jsonDoc(docObject);
    file.write(jsonDoc.toJson(QJsonDocument::Indented)); // Use Indented for readability
    file.close();

    setModified(false); // Clear the modified flag after saving
    m_isSaving = false; // Reset the guard flag AFTER marking as unmodified
    qDebug() << "DocumentWidget save: Setting m_isSaving to false.";
    qDebug() << "DocumentWidget save: Successfully saved" << m_filePath << "Modified:" << isModified();
    return true;
}

// Exports the document as PDF.
bool DocumentWidget::exportPDF(const QString &filePath) {
    if (filePath.isEmpty()) {
        qDebug() << "DocumentWidget exportPDF: No file path specified.";
        return false;
    }
    
    QPdfWriter pdf(filePath);
    
    // A4 page, portrait orientation.
    pdf.setPageSize(QPageSize(QPageSize::A4));
    pdf.setPageOrientation(QPageLayout::Portrait);
    
    // Use a reasonable PDF resolution.
    pdf.setResolution(96);
    
    QPainter painter(&pdf);
    
    if (!painter.isActive()) {
        qDebug() << "DocumentWidget exportPDF: Could not start PDF painter.";
        return false;
    }
    
    /*
     * Save the current selection because exporting should not change
     * the user's selection state.
     */
    QList<QGraphicsItem *> selectedItems = m_graphicsScene->selectedItems();
    
    m_graphicsScene->clearSelection();
    const QSizeF pageSize = m_graphicsScene->pageSizePx();
    const int pageCount = m_graphicsScene->pageCount();
    
    for (PageA4Item *page : m_graphicsScene->getPages())
        page->hide();
    
    for (int page = 0; page < pageCount; ++page)
    {
        if (page > 0)
            pdf.newPage();
        
        // Scene rectangle corresponding to this A4 page.
        const QRectF sourceRect(0.0, page * pageSize.height(), pageSize.width(), pageSize.height());
        
        // Render the scene page into the entire PDF page.
        const QRectF targetRect = painter.viewport();
        
        m_graphicsScene->render(&painter, targetRect, sourceRect, Qt::KeepAspectRatio);
    }
    
    painter.end();
    
    for (PageA4Item *page : m_graphicsScene->getPages())
        page->show();
    
    // Restore the selection.
    for (QGraphicsItem *item : selectedItems) {
        if (item)
            item->setSelected(true);
    }
    
    qDebug() << "DocumentWidget exportPDF: Successfully exported" << filePath;
    
    return true;
}
// Loads the document from its current file path.
bool DocumentWidget::load() {
    if (m_filePath.isEmpty()) {
        qDebug() << "DocumentWidget load: No file path set for" << this;
        return false;
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "MathDoc",
                             QString("Could not read file %1:\n%2")
                                 .arg(m_filePath, file.errorString()));
        qDebug() << "DocumentWidget load: Failed to open file" << m_filePath << ":" << file.errorString();
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        QMessageBox::warning(this, "MathDoc",
                             QString("Error parsing document %1:\n%2")
                                 .arg(m_filePath, parseError.errorString()));
        qDebug() << "DocumentWidget load: JSON parse error for" << m_filePath << ":" << parseError.errorString();
        return false;
    }

    if (!jsonDoc.isObject()) {
        QMessageBox::warning(this, "MathDoc",
                             QString("Invalid document format for %1: Expected JSON object.")
                                 .arg(m_filePath));
        qDebug() << "DocumentWidget load: Invalid JSON format for" << m_filePath;
        return false;
    }

    QJsonArray itemsArray = jsonDoc.array();
    /*
    m_graphicsScene->clearContent(); // Clear existing content before loading new
    
    for (const QJsonValue &itemValue : itemsArray) {
        QJsonObject itemObject = itemValue.toObject();
        int type = itemObject["type"].toInt();
        
        if (type == PageTextItem::Type) {
            PageTextItem* textItem = PageTextItem::fromJson(itemObject);
            m_graphicsScene->addItem(textItem);
        } else if (type == PageMathItem::Type) { //
            PageMathItem* mathItem = PageMathItem::fromJson(itemObject, m_data); //
            m_graphicsScene->addItem(mathItem); //
        } else if (type == PageA4Item::Type) {
            // A4 Pages are created automatically, so we skip these
        }
    }*/
    
    m_graphicsScene->clearContent(); // Clear existing content before loading new

    QJsonObject docObject = jsonDoc.object();
    if (docObject.contains("frames") && docObject["frames"].isArray()) {
        QJsonArray framesArray = docObject["frames"].toArray();
        for (const QJsonValue &value : framesArray) {
            QJsonObject frameObject = value.toObject();
            int type = frameObject["type"].toInt();
            
            if (type == PageTextItem::Type) {
                PageTextItem* textItem = PageTextItem::fromJson(frameObject);
                m_graphicsScene->addItem(textItem);
            } else if (type == PageMathItem::Type) { //
                bool isInDiagramTitle = frameObject["isInDiagramTitle"].toBool();
                if (!isInDiagramTitle) {
                    PageMathItem* mathItem = PageMathItem::fromJson(frameObject, m_data); //
                    m_graphicsScene->addItem(mathItem); //
                    mathItem->updateLayout();
                }
            } else if (type == PageDiagramItem::Type) { //
                PageDiagramItem* diagramItem = PageDiagramItem::fromJson(frameObject, m_data); //
                m_graphicsScene->addItem(diagramItem); //
            } else if (type == PageImageItem::Type) { //
                PageImageItem *imageItem = new PageImageItem();
                if (imageItem->fromJson(frameObject)) {
                    m_graphicsScene->addItem(imageItem);
                } else {
                    delete imageItem;
                }
            }
        }
    }
    
    // lowestY based on the bottom of the current item
    // Use sceneBoundingRect().bottom() for the actual bottom edge
    qreal lowestY = -1.0; // Track the lowest Y coordinate of any loaded item
    for (auto item: m_graphicsScene->textFrames()) {
        if (item->sceneBoundingRect().bottom() > lowestY) {
            lowestY = item->sceneBoundingRect().bottom();
        }
    }
    for (auto item: m_graphicsScene->mathFrames()) {
        if (item->sceneBoundingRect().bottom() > lowestY) {
            lowestY = item->sceneBoundingRect().bottom();
        }
    }
    
    // --- Page Creation (Bottom) - After loading all items ---
    // Calculate how many pages are required to contain the lowest item.
    QSizeF pageSizePx = m_graphicsScene->pageSizePx();
    int requiredPages = qCeil(lowestY / pageSizePx.height());
    
    // Ensure at least one page is present if content exists but doesn't fill a full page
    if (requiredPages == 0 && lowestY > 0) {
        requiredPages = 1;
    } else if (lowestY <= 0) { // If no items loaded or lowestY is zero/negative, ensure at least one page is there.
        requiredPages = 1; // Always have at least one page
    }
    
    // Add new pages until the scene has enough pages to contain all loaded items.
    while (m_graphicsScene->pageCount() < requiredPages) {
        m_graphicsScene->addA4Page(); // Call your scene's method to add a new A4 page.
        qDebug() << "DocumentWidget load: Added new page. Total pages:" << m_graphicsScene->pageCount();
    }
    
    m_graphicsScene->compute();
    
    setModified(false); // Clear the modified flag after loading
    qDebug() << "DocumentWidget load: Successfully loaded" << m_filePath << "Modified:" << isModified();
    
    m_graphicsScene->clearSelection();
    
    return true;
}

// Slot called when the content of the graphics scene changes.
void DocumentWidget::onSceneChanged(const QList<QRectF> &region) {
    Q_UNUSED(region);

    // If we are currently in the process of saving, do NOT mark as modified.
    // This prevents the infinite loop.
    if (m_isSaving) {
        qDebug() << "DocumentWidget onSceneChanged: Ignoring scene change during save operation.";
        return;
    }

    setModified(true); // Mark the document as modified
    // The setModified method already emits contentModified(), so no need to emit here again.
    // emit contentModified(); // REMOVED: Redundant, setModified already does this.
    qDebug() << "DocumentWidget onSceneChanged:" << this << "Modified:" << isModified();
}

// Slot called when the content of the graphics scene (items) changes.
void DocumentWidget::onDocumentContentItemsChanged() {
    // If we are currently in the process of saving, do NOT mark as modified.
    if (m_isSaving) {
        qDebug() << "DocumentWidget onDocumentContentItemsChanged: Ignoring content change during save operation.";
        return;
    }

    setModified(true); // Mark the document as modified
    qDebug() << "DocumentWidget onDocumentContentItemsChanged:" << this << "Modified:" << isModified();
}

void DocumentWidget::onCursorMoved(const QPointF &newCursorPos, qreal cursorHeight) {
    if (m_graphicsView) {
        // Create a QRectF for the cursor's bounding box
        QRectF cursorRect(newCursorPos, QSizeF(1, cursorHeight)); // Assuming cursor is 1px wide
        m_graphicsView->ensureVisible(cursorRect, 20, 20); // Add 20px margins for padding
    }
}
