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

// documentwidget.h
#ifndef DOCUMENTWIDGET_H
#define DOCUMENTWIDGET_H

#include <QWidget>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QJsonDocument> // For JSON serialization
#include <QJsonObject>   // For JSON objects
#include <QJsonArray>    // For JSON arrays
#include <QJsonValue>    // For JSON values

#include "pagegraphicsview.h" // Include our new graphics view
#include "pagegraphicsscene.h" // Include our new graphics scene
#include "pagetextitem.h"      // Include our new text item
#include "data.h"

// Forward declaration of MainWindow
class MainWindow;

// DocumentWidget class represents a single document (a .mad file).
// It now uses a PageGraphicsView to display pages and frames.
class DocumentWidget : public QWidget {
    Q_OBJECT // Macro required for any class that defines signals or slots

public:
    // Constructor: Takes the MainWindow, a parent widget and an optional file path.
    explicit DocumentWidget(MainWindow *mainWindow, QWidget *parent = nullptr, const QString &filePath = QString());

    // Returns the current file path of the document.
    QString filePath() const { return m_filePath; }

    // Sets the file path for the document.
    void setFilePath(const QString &path);

    // Returns true if the document has been modified since the last save.
    bool isModified() const; // Now checks internal flag

    // Saves the document to its current file path.
    // Returns true on success, false otherwise.
    bool save();
    bool exportPDF(const QString &filePath);
    
    // Loads the document from its current file path.
    // Returns true on success, false otherwise.
    bool load();

    // Clears the modified flag of the document.
    void setModified(bool modified); // Sets internal flag

    // Getter for the graphics view (needed for setting focus in MdiTabbedDocumentArea)
    PageGraphicsView* graphicsView() const { return m_graphicsView; }

signals:
    // Signal emitted when the document's content is modified.
    void contentModified();

private slots:
    // Slot to handle changes in the graphics scene (e.g., item moved/text changed). 
    void onSceneChanged(const QList<QRectF> &region);
    // Slot to handle changes in actual document content items.
    void onDocumentContentItemsChanged();
    void onCursorMoved(const QPointF &newCursorPos, qreal cursorHeight);
    
private:
    MainWindow *m_MainWindow;
    PageGraphicsView *m_graphicsView;   // The view for displaying pages and items
    PageGraphicsScene *m_graphicsScene; // The scene containing pages and items
    QString m_filePath;                 // The full path to the file associated with this document
    bool m_isModified;                  // Internal flag to track if the document has been modified
    bool m_isSaving = false;            // Flag to indicate if a save operation is in progress
    Data *m_data;
    
};

#endif // DOCUMENTWIDGET_H
