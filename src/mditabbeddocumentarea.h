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

// mditabbeddocumentarea.h
#ifndef MDITABBEDDOCUMENTAREA_H
#define MDITABBEDDOCUMENTAREA_H

#include <QMdiSubWindow>
#include <QTabWidget>
#include <QList>
#include <QString>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QCloseEvent>

#include "documentwidget.h" // Include our custom document widget
// Forward declaration of MainWindow
class MainWindow;


// MdiTabbedDocumentArea is a QMdiSubWindow that contains a QTabWidget.
// Each tab in the QTabWidget holds a DocumentWidget.
// This class represents one "partition" of the main window.
class MdiTabbedDocumentArea : public QMdiSubWindow {
    Q_OBJECT

public:
    // Constructor: Takes a parent widget.
    explicit MdiTabbedDocumentArea(MainWindow *mainWindow, QWidget *parent = nullptr);
    ~MdiTabbedDocumentArea(); // Explicit destructor declaration

    // Creates a new, empty document and adds it as a new tab.
    DocumentWidget* newDocument();

    // Opens an existing document from a file and adds it as a new tab.
    DocumentWidget* openDocument(const QString &filePath = QString());

    // Saves the currently active document in the active tab.
    // Prompts for "Save As" if the document is new/unsaved.
    bool saveCurrentDocument();

    // Saves the currently active document to a new file path (Save As).
    bool saveCurrentDocumentAs();
    
    // Exports the currently active document to a pdf with the same name.
    bool exportCurrentDocumentAsPDF();
    
    // Closes the currently active document.
    // Prompts to save if modified.
    bool closeCurrentDocument();

    // Returns the DocumentWidget currently displayed in the active tab.
    DocumentWidget* currentDocument() const;

    // Returns the QTabWidget managed by this sub-window.
    QTabWidget* tabWidget() const { return m_tabWidget; }

public slots: // Moved from private slots to public slots to allow external calls
    // Slot called when the current tab changes. Updates the sub-window title.
    void onCurrentTabChanged(int index);

protected:
    // Overrides the close event to handle unsaved documents before closing the sub-window.
    void closeEvent(QCloseEvent *event) override;

signals: // Signal to notify parent (MainWindow) about document modification status changes
    void documentModificationStatusChanged();

private slots:
    // Slot called when a tab is requested to be closed by the user (e.g., clicking 'x' on tab).
    void onTabCloseRequested(int index);

    // Slot called when a document's content is modified. Updates the tab title.
    void onDocumentContentModified();

private:
    QTabWidget *m_tabWidget; // The tab widget holding the documents
    MainWindow *m_MainWindow;

    // Helper function to create a new tab for a DocumentWidget.
    void addDocumentTab(DocumentWidget *docWidget);

    // Helper function to prompt the user to save if a document is modified.
    // Returns true if the user saved or chose not to save, false if they cancelled.
    bool maybeSaveDocument(DocumentWidget *doc);
};

#endif // MDITABBEDDOCUMENTAREA_H
