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

// mathleafedit.cpp
#include "mathleafedit.h"
#include "pagemathitem.h"
#include "helpers.h"
#include <QGraphicsScene>
#include <QGraphicsObject>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QTextDocument>
#include <QDebug>
#include <QGraphicsTextItem>
#include <QFontMetrics>
#include <QFontMetricsF>
#include <QMetaType>
#include <QFocusEvent>
#include <QCursor>
#include <QGraphicsSceneHoverEvent>
#include <QTextCursor>
#include <QCoreApplication>
#include <QFontDatabase>
#include <QRawFont>
#include <QJsonArray>
#include <QJsonObject>
#include <QException>

MathLeafEdit::MathLeafEdit(Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent)
: QGraphicsObject(parent), m_data(d), m_parentPageMathItem(parentPageMathItem) {
    updateContStr();
    setFlags(ItemIsFocusable);
    setAcceptHoverEvents(true);
    
    m_leftParenthesisPixmap = QPixmap();
    m_rightParenthesisPixmap = QPixmap();
    m_leftBracesPixmap = QPixmap();
    m_rightBracesPixmap = QPixmap();
    
    // Initialize QGraphicsPathItems for parentheses
    m_leftParenthesis = new QGraphicsPathItem(this);
    m_rightParenthesis = new QGraphicsPathItem(this);
    m_leftBraces = new QGraphicsPathItem(this);
    m_rightBraces = new QGraphicsPathItem(this);
    m_leftParenthesis->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    m_rightParenthesis->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    m_leftBraces->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    m_rightBraces->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    m_leftParenthesis->setAcceptHoverEvents(false);
    m_rightParenthesis->setAcceptHoverEvents(false);
    m_leftBraces->setAcceptHoverEvents(false);
    m_rightBraces->setAcceptHoverEvents(false);
    if(m_paintParenthesisFast) {
        m_leftParenthesis->setPen(QPen(QColor(0, 0, 191),1));
        m_leftParenthesis->setBrush(Qt::NoBrush);
        m_rightParenthesis->setPen(QPen(QColor(0, 0, 191),1));
        m_rightParenthesis->setBrush(Qt::NoBrush);
    } else {
        m_leftParenthesis->setPen(QPen(Qt::NoPen));
        m_leftParenthesis->setBrush(QColor(0, 0, 191));
        m_rightParenthesis->setPen(QPen(Qt::NoPen));
        m_rightParenthesis->setBrush(QColor(0, 0, 191));
    }
    if(m_paintBracesFast) {
        m_leftBraces->setPen(QPen(QColor(0, 0, 191),1));
        m_leftBraces->setBrush(Qt::NoBrush);
        m_rightBraces->setPen(QPen(QColor(0, 0, 191),1));
        m_rightBraces->setBrush(Qt::NoBrush);
    } else {
        m_leftBraces->setPen(QPen(Qt::NoPen));
        m_leftBraces->setBrush(QColor(0, 0, 191));
        m_rightBraces->setPen(QPen(Qt::NoPen));
        m_rightBraces->setBrush(QColor(0, 0, 191));
    }
    m_leftParenthesis->hide();
    m_rightParenthesis->hide();
    m_leftBraces->hide();
    m_rightBraces->hide();
    
    connect(&m_cursorTimer, &QTimer::timeout, this, &MathLeafEdit::toggleCursor);
    connect(this, &MathLeafEdit::simulateKeyPress, this, &MathLeafEdit::handleSimulatedKeyPress);
    MathLeafEdit* parentMathLeafEdit = qobject_cast<MathLeafEdit*>(parentObject());
    if (parentMathLeafEdit) {
        connect(this, &MathLeafEdit::transferContentTo, parentMathLeafEdit, &MathLeafEdit::onTransferContentTo);
        // Set a default font and size
        m_mathFontSize = parentMathLeafEdit->mathFontSize();
        m_fontName = parentMathLeafEdit->fontName();
    }
    
    QTimer* timerForUpdateBoundingRectAndLayout = new QTimer(this);
    connect(timerForUpdateBoundingRectAndLayout, &QTimer::timeout, this, &MathLeafEdit::refreshLayout);
    timerForUpdateBoundingRectAndLayout->start(1000); // Start a timer that ticks every 10000ms (10 seconds)
    
    QFont font(m_fontName, m_mathFontSize);
    setFont(font);
    setSelected(true);
    setFocus();
    //m_content.append(QString());
    // Initial call to set up the correct size
    updateBoundingRectAndLayout();
    setZValue(parent->zValue()+1);
    QMetaObject::invokeMethod(this, "refreshLayout", Qt::QueuedConnection);
}

MathLeafEdit::MathLeafEdit(QString text, Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent)
: MathLeafEdit(d, parentPageMathItem, parent) {
    maybeFunction(text);
    updateContStr();
}

MathLeafEdit::~MathLeafEdit() {
    qDeleteAll(childItems());
}

void MathLeafEdit::maybeFunction(QString text) {updateContStr();
    if(text == ":") {
        insertText(":="); m_isFunction = true;
        MathLeafEdit* newL = new MathLeafEdit(m_data, m_parentPageMathItem, this);
        insertMathLeaf(newL);
        newL->updateBoundingRectAndLayout();
    } else if (text == "=") {
        insertText("="); m_isFunction = true;
        MathLeafEdit* newL = new MathLeafEdit(resultString(true), m_data, m_parentPageMathItem, this);
        insertMathLeaf(newL);
        newL->updateBoundingRectAndLayout();
    } else if (text == "*" || text == " " || text == longEqualChar ||
               text == "<" || text == smallerEqualChar ||
               text == ">" || text == greaterEqualChar) {
        m_paddingH = 0;
        QString tmpStr;
        if ( text == "*" ) tmpStr = cdotChar; else tmpStr = text;
        insertText(tmpStr);
        m_isFunction = true;
    } else if (text == "/") {
        MathLeafEdit* numerator = new MathLeafEdit(m_data, m_parentPageMathItem, this);
        insertMathLeaf(numerator);
        emit transferContentTo(numerator);
        insertText(text);
        MathLeafEdit* denominator = new MathLeafEdit(m_data, m_parentPageMathItem, this);
        insertMathLeaf(denominator); denominator->setFocus();
        denominator->updateBoundingRectAndLayout();
        m_isFunction = true;
    } else if (text == "^") {
        MathLeafEdit* base = new MathLeafEdit(m_data, m_parentPageMathItem, this);
        qreal baseFontSize = base->mathFontSize();
        insertMathLeaf(base);
        emit transferContentTo(base);
        insertText(text);
        MathLeafEdit* exponent = new MathLeafEdit(m_data, m_parentPageMathItem, this);
        qreal expoFontSize = exponent->mathFontSize();
        insertMathLeaf(exponent); exponent->setFocus();
        exponent->setFontSize(m_mathFontSize*m_subScriptScale);
        base->updateBoundingRectAndLayout();
        exponent->updateBoundingRectAndLayout();
        m_isFunction = true;
    } else if (text == "+" || text == "-" || text == "&" || text == "|") {
        insertText(text);
        m_isFunction = true;
    } else if (text.back() == '(') {
        if (text == "root(") {
            insertText(text);
            m_isFunction = true;
            checkForValidFunction();
            checkForParenthesis();
//            if(m_isValidFunction) qDebug() << "MathLeafEdit::maybeFunction: 'root(' m_isValidFunction = " << m_isValidFunction;
            MathLeafEdit *radical = new MathLeafEdit(m_data, m_parentPageMathItem, this);// radical index of root
            insertMathLeaf(radical); // insert the radical on top of root
            radical->setFontSize(0.8*m_mathFontSize*m_subScriptScale); // set radical font sizte smaller
            radical->insertText("2");
            radical->updateBoundingRectAndLayout();
            insertMathLeaf(new MathLeafEdit(m_data, m_parentPageMathItem, this)); // insert a field under root (=radicand)
            updateContStr();
        } else if (text == "log(") {
            insertText(text);
            m_isFunction = true;
            checkForValidFunction();
            checkForParenthesis();
//            if(m_isValidFunction) qDebug() << "MathLeafEdit::maybeFunction: 'log(' m_isValidFunction = " << m_isValidFunction;
            MathLeafEdit *base = new MathLeafEdit(m_data, m_parentPageMathItem, this);// base of log
            insertMathLeaf(base); // insert the base of the log
            base->setFontSize(0.8*m_mathFontSize*m_subScriptScale); // set base font sizte smaller
            base->insertText("1"); base->insertText("0");
            base->boundingRect();
            MathLeafEdit* newL = new MathLeafEdit(m_data, m_parentPageMathItem, this);
            insertMathLeaf(newL); // insert argument inside log( )
            updateContStr();
            newL -> updateBoundingRectAndLayout();
        } else if (text == "(") {
            insertText(text);
            m_isFunction = true;
            checkForValidFunction();
            checkForParenthesis();
//            if(m_isValidFunction) qDebug() << "MathLeafEdit::maybeFunction: '(' m_isValidFunction = " << m_isValidFunction;
            MathLeafEdit* inParenthesis = new MathLeafEdit(m_data, m_parentPageMathItem, this);
            insertMathLeaf(inParenthesis);
            emit transferContentTo(inParenthesis);
            inParenthesis->setFocus();
            inParenthesis->updateBoundingRectAndLayout();
        } else {
            insertText(text);
            m_isFunction = true;
            checkForValidFunction();
            checkForParenthesis();
//            if(m_isValidFunction) qDebug() << "MathLeafEdit::maybeFunction: '" << text << "' m_isValidFunction = " << m_isValidFunction;
            MathLeafEdit* newL = new MathLeafEdit(m_data, m_parentPageMathItem, this);
            insertMathLeaf(newL);
            updateContStr();
            newL->updateBoundingRectAndLayout();
        }
    } else if (text == "{") {
        m_showBraces = true;
        m_isPiecewiseFunction = true;
        insertText(text);
        MathLeafEdit* newBool = new MathLeafEdit(m_data, m_parentPageMathItem, this);
        MathLeafEdit* newFunc = new MathLeafEdit(m_data, m_parentPageMathItem, this);
        insertMathLeaf(newBool);
        insertText(";");
        insertMathLeaf(newFunc);
        updateContStr();
        newBool->updateBoundingRectAndLayout();
        newFunc->updateBoundingRectAndLayout();
        newBool->setFocus();
    } else {
        insertText(text);
    }
}

QRectF MathLeafEdit::updateBoundingRect() {
    updateContStr();
    // The bounding rect should not be empty, so it always contains the minimum cursor size
    QFontMetricsF fm(m_font);
    QFont subscriptFont(m_fontName, m_mathFontSize*m_subScriptScale);
    //QFont subscriptFont(m_fontName, fm.height()*m_subScriptScale);
    QFontMetricsF subFm(subscriptFont);
    m_centerHeight = 0.5*fm.height();
    
    QRectF rect;
    if (m_content.size() == 0) {
        QImage dummy(1, 1, QImage::Format_ARGB32_Premultiplied);
        QPainter p(&dummy);
        p.setFont(m_font);
        QFontMetricsF fontMet = p.fontMetrics();
        rect = QRectF(0, 0, fontMet.horizontalAdvance(" "), fontMet.height());
        return rect;
    }
    // division has completely different bounding rect, so we need
    // to make sure we are inside a division
    QString contStr = "";
    for(int i=0; i<m_content.size(); i++){
        if (isStringAtContent(i)) {
            QString str = getStringAtContent(i);
            if (str.isEmpty()) 
                contStr = contStr + QString("<EMPTY>");
            else
                contStr = contStr + str;
        } else if (isMathLeafEditAtContent(i)) {
            contStr = contStr + QString("<OBJ>");
        }
    }
    if(contStr == "<OBJ>/<OBJ>" && m_isFunction) {
        MathLeafEdit* numer = getMathLeafEditAtContent(0);
        MathLeafEdit* denom = getMathLeafEditAtContent(2);
        if (numer && denom) {
            qreal numW = numer->boundingRect().width();
            qreal numH = numer->boundingRectangle().height();
            qreal denW = denom->boundingRect().width();
            qreal denH = denom->boundingRectangle().height();
            qreal maxW = qMax(numW, denW);
            rect = QRectF(0,0,maxW, numH + denH + 4*m_paddingV);
            m_centerHeight = numH+2*m_paddingV;
        }
    } else if(contStr == "<OBJ>^<OBJ>" && m_isFunction) {
        MathLeafEdit* base = getMathLeafEditAtContent(0);
        MathLeafEdit* expo = getMathLeafEditAtContent(2);
        if (base && expo) {
            qreal baseW = base->boundingRect().width();
            qreal baseH = base->boundingRectangle().height();
            qreal expoW = expo->boundingRect().width();
            qreal expoH = expo->boundingRectangle().height();
            
            // Position of base and exponent rectangles (must be same as in updateChildPositions() )
            qreal baseX = 0.0; qreal baseY = qMax(expoH - 0.4*baseH, 0.5*expoH);
            qreal expoX = baseW; qreal expoY = 0.0;
            
            rect = QRectF(0,0,baseW+expoW+2*m_paddingH, baseY+baseH+2*m_paddingV);
            m_centerHeight = baseY+base->centerHeight()+m_paddingV;
        }
    } else if(contStr == "root(<OBJ><OBJ>" && m_isFunction) {
        MathLeafEdit* radical = getMathLeafEditAtContent(1);
        MathLeafEdit* radicand = getMathLeafEditAtContent(2);
        if (radical && radicand) {
            qreal radicalW = radical->boundingRect().width();
            qreal radicalH = radical->boundingRectangle().height();
            qreal radicandW = radicand->boundingRect().width();
            qreal radicandH = radicand->boundingRectangle().height();
            
            // Position of base and exponent rectangles (must be same as in updateChildPositions() )
            qreal radicalX = 0.0; qreal radicalY = 0.0;
            qreal radicandX = radicalW + fm.horizontalAdvance("\\/"); qreal radicandY = 2*m_paddingV;
            
            rect = QRectF(0,0,radicandX+radicandW+3*m_paddingH, radicandY + radicandH);
            m_centerHeight = radicandY+radicand->centerHeight()+m_paddingV;
        }
    } else if(contStr == "log(<OBJ><OBJ>" && m_isFunction) {
        MathLeafEdit* base = getMathLeafEditAtContent(1);
        MathLeafEdit* arg = getMathLeafEditAtContent(2);
        if (base && arg) {
            qreal baseW = base->boundingRect().width();
            qreal baseH = base->boundingRectangle().height();
            qreal argW = arg->boundingRect().width();
            qreal argH = arg->boundingRectangle().height();
            
            // Position of base and exponent rectangles (must be same as in updateChildPositions() )
            qreal baseX = fm.horizontalAdvance("log"); qreal baseY = 0.8*fm.height()+m_paddingV;
            qreal argX = baseX + fm.horizontalAdvance("log("); qreal argY = m_paddingV;
            
            rect = QRectF(0, 0, argX + argW + 3*m_paddingH, argH + 2*m_paddingV);
            m_centerHeight = arg->centerHeight()+m_paddingV;
        }
    } else if(m_contStr == "mod(<OBJ><OBJ>" && m_isFunction) {
        MathLeafEdit* numer = getMathLeafEditAtContent(1);
        MathLeafEdit* denom = getMathLeafEditAtContent(2);
        if (numer && denom) {
            qreal numerW = numer->boundingRect().width();
            qreal numerH = numer->boundingRectangle().height();
            qreal denomW = denom->boundingRect().width();
            qreal denomH = denom->boundingRectangle().height();
            m_centerHeight = qMax(numer->centerHeight(), denom->centerHeight())+m_paddingV;
            
            // Position of numer and denom rectangles (must be same as in updateChildPositions() )
            qreal numerX = m_paddingH; qreal numerY = m_centerHeight-0.5*numerH-m_paddingV;
            qreal denomX = numerX + m_paddingH; qreal denomY = m_centerHeight-0.5*denomH-m_paddingV;
            
            rect = QRectF(0, 0, denomX + denomW + 3*m_paddingH, qMax(numerH, denomH) + 2*m_paddingV);
        }
    } else if(m_contStr[0] == '{' && m_isPiecewiseFunction) {
        //m_contStr == "{<OBJ>;<OBJ>"
        qreal lWidthMax = 0.0; qreal cWidth = 0.0;
        qreal lHeightMax = 0.0;
        for (int i=0; i<m_content.size(); i++) {
            if (isStringAtContent(i)) {
                if ( getStringAtContent(i) == "{" ) {
                    cWidth += fm.horizontalAdvance(getStringAtContent(i));
                    cWidth += fm.horizontalAdvance(";");
                }
                if ( getStringAtContent(i) == ";" ) {
                    MathLeafEdit* mleBool = getMathLeafEditAtContent(i-1);
                    MathLeafEdit* mleFunc = getMathLeafEditAtContent(i+1);
                    qreal lWidth = cWidth + mleBool->boundingRectangle().width() + mleFunc->boundingRectangle().width();
                    qreal lHeight = qMax(mleBool->boundingRectangle().height(),
                                         mleFunc->boundingRectangle().height());
                    lWidthMax = qMax(lWidthMax, lWidth);
                    lHeightMax += lHeight;
                }
            }
        }
        rect = QRectF(0.0, 0.0, lWidthMax + 2.0*m_paddingH, lHeightMax);
    } else {
        qreal currentX = m_paddingH;
        qreal maxY = fm.height();
        bool subscriptActive = false;
        
        for (int i=0; i< m_content.size(); i++) {
            if (isStringAtContent(i)) {
                QString text = getStringAtContent(i);
                if(text == "_" && !subscriptActive) { subscriptActive = true; continue; }
                if(subscriptActive) currentX += subFm.horizontalAdvance(text);
                else currentX += fm.horizontalAdvance(text);
                maxY = qMax(maxY, fm.height() + 3.0*subscriptActive);
                m_centerHeight = qMax(m_centerHeight,0.5*maxY);
            } else if (isMathLeafEditAtContent(i)) {
//                if (m_contStr == "<OBJ>/")
//                    qDebug() << "MathLeafEdit::boundingRect: m_contStr = " << m_contStr;
                subscriptActive = false;
                MathLeafEdit* child = getMathLeafEditAtContent(i);
                currentX += child->boundingRect().width() + m_paddingH/3.0;
                maxY = qMax(maxY, child->boundingRect().height());
                m_centerHeight = qMax(m_centerHeight,child->centerHeight());
            }
        }
        if (m_showParenthesis) { currentX += fm.horizontalAdvance(")"); maxY += 2*m_paddingV; }
        if (m_contStr.isEmpty()) { currentX = 5.0*m_paddingH; maxY = 1.3*fm.height(); }
        //currentX-1, 0, fm.horizontalAdvance(text)+2, m_textBoundingRect.height() das hier wird für highlighting verwendet!
        rect =  QRectF(0, 0, currentX, maxY);
        //m_centerHeight = 0.5*maxY;
    }
    m_boundingRectangle = rect;
    return rect;
}

void MathLeafEdit::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);
    updateContStr();
    //if (m_contStr == QString("c")) { qDebug() << "MathLeafEdit::paint: m_contStr = " << m_contStr << "  # " << m_count; m_count++; }
    
    if (m_contStr == "<OBJ>/<OBJ>" && m_isFunction) { // for a division we go to a seperate paint function
        paintDivision(painter);
        return;
    } else if (m_contStr == "<OBJ>^<OBJ>" && m_isFunction) {
        if (hasFocus()) {
            painter->setPen(QPen(Qt::blue, 1, Qt::DotLine));
            painter->setBrush(QColor(255, 255, 127)); // Qt::lightyellow does not exist
            painter->drawRect(m_boundingRectangle);
        }
        return;
    } else if (m_contStr == "root(<OBJ><OBJ>" && m_isFunction) {
        paintRoot(painter);
        return;
    } else if ( m_isPiecewiseFunction && m_showBraces) {
        paintPiecewiseFunction(painter);
        return;
    }
    
    painter->setRenderHint(QPainter::Antialiasing, false);
    
    if (hasFocus()) {
        painter->setPen(QPen(Qt::blue, 1, Qt::DotLine));
        painter->setBrush(QColor(255, 255, 127)); // Qt::lightyellow does not exist
        painter->drawRect(m_boundingRectangle);
//        qDebug() << "boundingRect() = " << m_boundingRectangle;
    } else {
        painter->setPen(Qt::NoPen);
        painter->setBrush(Qt::NoBrush);
        painter->setBrush(Qt::white);
        painter->drawRect(m_boundingRectangle);
    }
    
    if (m_isHighlighted) { // make Background light blue if it is included into a selection
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(173, 216, 230)); // Light blue highlight
        painter->drawRect(m_boundingRectangle);
    }
    if (m_contStr.isEmpty()) { // make Background light gray if it is included into a selection
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(230, 230, 230)); // Light blue highlight
        painter->drawRect(m_boundingRectangle);
    }
    
    QBrush bgBrush(painter->brush());
    painter->setPen(QPen()); // set Pen to default
    
    qreal currentX = m_paddingH; // Use local coordinates
    QFontMetricsF fm(m_font);
    QFont subscriptFont(m_fontName, m_mathFontSize*m_subScriptScale);
    bool subscriptActive = false;
    for (int i = 0; i < m_content.size(); ++i) {
        // Draw cursor if visible and at the current position
        if (m_cursorVisible && i == m_cursorPos) {
            painter->setPen(QPen(Qt::black, 1));
            m_cursorX = currentX-1;  m_cursorY = m_centerHeight - 0.5*m_textBoundingRect.height();
            m_cursorW = 2;           m_cursorH = m_textBoundingRect.height();
            painter->drawLine(currentX, 0, currentX, m_textBoundingRect.height());
            painter->setPen(QPen());
        }
        
        if (isStringAtContent(i)) { // handle text content
            QString text = getStringAtContent(i);
            bool isDivision = false;
            if (!text.isEmpty()) {
                if(text == "_" && !subscriptActive) subscriptActive = true;
                else {
                    // Check if this text segment is part of the selection
                    bool isSelected = (i >= qMin(m_selectionStart, m_selectionEnd) && i < qMax(m_selectionStart, m_selectionEnd));
                    
                    if (isSelected && hasFocus()) {
                        painter->setBrush(QColor(173, 216, 230));
                        painter->setPen(Qt::NoPen);
                        painter->drawRect(currentX-1, 0, fm.horizontalAdvance(text)+2, m_textBoundingRect.height());
                        painter->setBrush(Qt::NoBrush); painter->setPen(QPen());
                    }
                    if ( m_isValidFunction && m_isFunction ) {
                        QPen arcPen(QColor(0, 0, 191));
                        arcPen.setCapStyle(Qt::RoundCap);
                        painter->setPen(arcPen);
                    }
                    if (subscriptActive) painter->setFont(subscriptFont); else painter->setFont(m_font);
                    //m_centerHeight - child->centerHeight() - m_paddingV
                    qreal textY = (m_centerHeight - 0.5*fm.height()) + 7.0 * subscriptActive;
                    if( ((text.length()>1 && text.back() == '(') || text == "(") &&
                        m_isFunction && m_isValidFunction) text.chop(1);
                    
                    //painter->drawText(QPointF(currentX, textY), Qt::AlignLeft | Qt::AlignTop, text);
                    painter->drawText(QRectF(currentX, textY,fm.horizontalAdvance(text),fm.height()),
                                      Qt::AlignLeft | Qt::AlignTop, text);
                    qreal horizontalAdvance;
                    if (subscriptActive) {
                        horizontalAdvance = QFontMetricsF(subscriptFont).horizontalAdvance(text);
                    } else {
                        horizontalAdvance = fm.horizontalAdvance(text);
                    }
                    currentX += horizontalAdvance;
                    if ( m_isValidFunction && m_isFunction && m_showParenthesis ) { // Draw a ( ) around the content of the function
                        if (i<m_content.size()-1) { if (isMathLeafEditAtContent(i+1)) {
                            MathLeafEdit* fContent = getMathLeafEditAtContent(i+1);
                            qreal h = fContent->boundingRectangle().height();
                            qreal d = fm.horizontalAdvance("(");
                            currentX += m_leftParenthesis->boundingRect().width();
                            if (m_contStr == "log(<OBJ><OBJ>")
                                m_leftParenthesis->setPos(fContent->boundingRectangle().width()+m_paddingH, 0);
                            else
                                m_leftParenthesis->setPos(0, 0);
                            m_leftParenthesis->show();
                        }
                        }
                    }
                }
            }
        } else if (isMathLeafEditAtContent(i)) {
            subscriptActive = false;
            MathLeafEdit* child = getMathLeafEditAtContent(i);
            bool isSelected = (i >= qMin(m_selectionStart, m_selectionEnd) && i < qMax(m_selectionStart, m_selectionEnd));
            (isSelected)? child->setHighlighted(true) : child->setHighlighted(false);
            currentX += child->boundingRectangle().width() + m_paddingH/3.0;
            if ( m_isValidFunction && m_isFunction && m_showParenthesis) { // Draw a ( ) around the content of the function
                if (i>0){
                    m_rightParenthesis->setPos(0, 0);
                    m_rightParenthesis->show();
                }
            }
        }
    }
    
    // Draw cursor at the very end if it's the last position
    if (m_cursorVisible && m_cursorPos == m_content.size()) {
        painter->setPen(QPen(Qt::black, 1));
        m_cursorX = currentX-1; m_cursorY = 0.0; m_cursorW = 2; m_cursorH = m_textBoundingRect.height();
        painter->drawLine(currentX, 0, currentX, m_textBoundingRect.height());
    }
}

void MathLeafEdit::paintDivision(QPainter* painter) {
    updateContStr();
    painter->save();
    
    if(!(m_contStr == "<OBJ>/<OBJ>" && m_isFunction)) { // for a division we go to a seperate paint function
        qWarning() << "MathLeafEdit::paintDivision: This is no division!";
        return;
    }
    painter->setRenderHint(QPainter::Antialiasing, false);
    
    if (hasFocus()) {
        painter->setPen(QPen(Qt::blue, 1, Qt::DotLine));
        painter->setBrush(QColor(255, 255, 127)); // Qt::lightyellow does not exist
        painter->drawRect(m_boundingRectangle);
    } else {
        painter->setPen(Qt::NoPen);
        painter->setBrush(Qt::NoBrush);
        painter->setBrush(Qt::white);
        painter->drawRect(m_boundingRectangle);
    }
    
    if (m_isHighlighted) { // make Background light blue if it is included into a selection
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(173, 216, 230)); // Light blue highlight
        painter->drawRect(m_boundingRectangle);
    }
    
    QBrush bgBrush(painter->brush());
    painter->setPen(QPen(QColor(0, 0, 0), 1.3*m_mathFontSize/12.0)); // set Pen to black
    
    MathLeafEdit* numer = getMathLeafEditAtContent(0);
    MathLeafEdit* denom = getMathLeafEditAtContent(2);
    numer->updateContStr(); denom->updateContStr();
    //qDebug() << "MathLeafEdit::paintDivision: " << numer->contStr() << "/" << denom->contStr();
    //qDebug() << "    QrectF = " << numer->m_boundingRectangle << "/" << denom->m_boundingRectangle;
    if (numer && denom) {
        qreal numW = numer->boundingRectangle().width();
        qreal numH = numer->boundingRectangle().height();
        qreal denW = denom->boundingRectangle().width();
        qreal maxW = qMax(numW, denW);
        painter->drawLine(0, numH+2.75*m_paddingV, maxW, numH+2.75*m_paddingV);
    }
    painter->restore();
}

void MathLeafEdit::paintRoot(QPainter* painter) {
    updateContStr();
    painter->save();
    QFontMetricsF fm(m_font);
    
    if(!(m_contStr == "root(<OBJ><OBJ>" && m_isFunction)) { // for a division we go to a seperate paint function
        qWarning() << "MathLeafEdit::paintDivision: This is no division!";
        return;
    }
    painter->setRenderHint(QPainter::Antialiasing, false);
    
    if (hasFocus()) {
        painter->setPen(QPen(Qt::blue, 1, Qt::DotLine));
        painter->setBrush(QColor(255, 255, 127)); // Qt::lightyellow does not exist
        painter->drawRect(m_boundingRectangle);
    } else {
        painter->setPen(Qt::NoPen);
        painter->setBrush(Qt::NoBrush);
        painter->setBrush(Qt::white);
        painter->drawRect(m_boundingRectangle);
    }
    
    if (m_isHighlighted) { // make Background light blue if it is included into a selection
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(173, 216, 230)); // Light blue highlight
        painter->drawRect(m_boundingRectangle);
    }
    
    QBrush bgBrush(painter->brush());
    QPen pen(QColor(0, 0, 0), 1.3 * m_mathFontSize / 12.0);
    pen.setCapStyle(Qt::RoundCap);
    painter->setPen(pen);
    
    MathLeafEdit* radical = getMathLeafEditAtContent(1);
    MathLeafEdit* radicand = getMathLeafEditAtContent(2);
    radical->updateContStr(); radicand->updateContStr();
    //qDebug() << "MathLeafEdit::paintDivision: " << numer->contStr() << "/" << denom->contStr();
    //qDebug() << "    QrectF = " << numer->m_boundingRectangle << "/" << denom->m_boundingRectangle;
    if (radical && radicand) {
        qreal radicalW = radical->boundingRectangle().width();
        qreal radicalH = radical->boundingRectangle().height();
        qreal radicandW = radicand->boundingRectangle().width();
        qreal radicandH = radicand->boundingRectangle().height();
        qreal radicalLineY = radicalH+1.5*m_paddingV;
        
        qreal inc = 0.5*fm.horizontalAdvance("_");
        painter->drawLine(0.0, radicalLineY, radicalW, radicalLineY);
        painter->drawLine(radicalW, radicalLineY, radicalW+0.5*inc, m_boundingRectangle.height());
        painter->drawLine(radicalW+0.5*inc, m_boundingRectangle.height(), radicalW+inc, 0.5*m_paddingH);
        painter->drawLine(radicalW+inc, 0.5*m_paddingH, radicalW+inc + radicandW + 3*m_paddingH, 0.5*m_paddingH);
        
    }
    painter->restore();
}
void MathLeafEdit::paintPiecewiseFunction(QPainter* painter) {
    updateContStr();
    painter->save();
    QFontMetricsF fm(m_font);
    painter->setRenderHint(QPainter::Antialiasing, false);
    
    if (hasFocus()) {
        painter->setPen(QPen(Qt::blue, 1, Qt::DotLine));
        painter->setBrush(QColor(255, 255, 127)); // Qt::lightyellow does not exist
        painter->drawRect(m_boundingRectangle);
    } else {
        painter->setPen(Qt::NoPen);
        painter->setBrush(Qt::NoBrush);
        painter->setBrush(Qt::white);
        painter->drawRect(m_boundingRectangle);
    }
    
    if (m_isHighlighted) { // make Background light blue if it is included into a selection
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(173, 216, 230)); // Light blue highlight
        painter->drawRect(m_boundingRectangle);
    }
    
    painter->setPen(QPen());
    
    m_leftBraces->setPos(0, 0);
    m_leftBraces->show();
    
    qreal currentX = 0.0; // Use local coordinates
    qreal currentY = 0.0;
    int cLine = (m_cursorPos - 1) / 3;
    painter->setPen(QPen(Qt::black, 1));
    painter->setBrush(QColor(255, 255, 63)); // Light blue highlight
    QRectF lineCursorRect(0.0,(cLine-1)*fm.height(), m_boundingRectangle.width(), fm.height());
    painter->drawRect(lineCursorRect);
    painter->setPen(QPen());
    painter->setBrush(Qt::white);
    for (int i = 0; i < m_content.size(); ++i) {
        // Draw cursor if visible and at the current position
        if (m_cursorVisible && i == m_cursorPos) {
            painter->setPen(QPen(Qt::black, 1));
            m_cursorX = currentX-1;  m_cursorY = m_centerHeight - 0.5*fm.height();
            m_cursorW = 2;           m_cursorH = fm.height();
            painter->drawLine(currentX, m_cursorY, currentX, m_cursorY+m_cursorH);
            painter->setPen(QPen());
        }
        
        if (isStringAtContent(i)) { // handle text content
            QString text = getStringAtContent(i);
            bool isDivision = false;
            if (!text.isEmpty()) {
                // Check if this text segment is part of the selection
                bool isSelected = (i >= qMin(m_selectionStart, m_selectionEnd) && i < qMax(m_selectionStart, m_selectionEnd));
                if (isSelected && hasFocus()) {
                    painter->setBrush(QColor(173, 216, 230));
                    painter->setPen(Qt::NoPen);
                    painter->drawRect(currentX-1, 0, fm.horizontalAdvance(text)+2, m_textBoundingRect.height());
                    painter->setBrush(Qt::NoBrush); painter->setPen(QPen());
                }
                if (text == "{") {
                    currentX += 1.3*fm.horizontalAdvance(text); text.chop(1);
                } else if (text == ";") {
                    qreal centerHeight = qMax(getMathLeafEditAtContent(i-1)->centerHeight(),
                                              getMathLeafEditAtContent(i+1)->centerHeight() );
                    qreal lineHeight = qMax(getMathLeafEditAtContent(i-1)->boundingRectangle().height(),
                                            getMathLeafEditAtContent(i+1)->boundingRectangle().height() );
                    
                    qDebug() << "MathLeafEdit::paintPiecewiseFunction: i = " << i;
                    qDebug() << "MathLeafEdit::paintPiecewiseFunction: centerHeight = " << centerHeight;
                    qDebug() << "MathLeafEdit::paintPiecewiseFunction: lineHeight = " << lineHeight;
                    
                    qreal textY = currentY + centerHeight - 0.5*fm.height();
                    qDebug() << "MathLeafEdit::paintPiecewiseFunction: currentX = " << currentX;
                    qDebug() << "MathLeafEdit::paintPiecewiseFunction: textY = " << textY;
                    painter->drawText(QRectF(currentX, textY,fm.horizontalAdvance(text),fm.height()),
                                  Qt::AlignLeft | Qt::AlignTop, text);
                    currentX = 1.3*fm.horizontalAdvance("{") - getMathLeafEditAtContent(i+1)->boundingRect().width();
                    currentY += lineHeight;
                }
            }
        } else if (isMathLeafEditAtContent(i)) {
            MathLeafEdit* child = getMathLeafEditAtContent(i);
            bool isSelected = (i >= qMin(m_selectionStart, m_selectionEnd) && i < qMax(m_selectionStart, m_selectionEnd));
            (isSelected)? child->setHighlighted(true) : child->setHighlighted(false);
            currentX += child->boundingRect().width() + m_paddingH/3.0;
        }
    }
    painter->restore();
}

void MathLeafEdit::insertText(const QString& inText, bool force) {
    updateContStr();
//    qDebug() << "MathLeafEdit::insertText(QString) START, m_contStr = " << m_contStr;
    if(!m_isFunction || force){
        QString text = inText;
        if(m_cursorPos>0){
            if (isStringAtContent(m_cursorPos-1)) {
                if (getStringAtContent(m_cursorPos-1) == "\\") {
                    m_content.removeAt(m_cursorPos-1);
                    m_cursorPos--;
                    text = getGreekCharacter(inText);
                }
            }
        }
        m_content.insert(m_cursorPos, text);
        m_cursorPos++;
        m_selectionEnd = m_selectionStart = -1;
        //updateBoundingRectAndLayout();
    }
    updateContStr();
//    qDebug() << "MathLeafEdit::insertText(QString) END, m_contStr = " << m_contStr;
}
void MathLeafEdit::insertText(QKeyEvent* event) {
    updateContStr();
//    qDebug() << "MathLeafEdit::insertText(QKeyEvent) START, m_contStr = " << m_contStr;
    if(!m_isFunction){
        QString text = event->text();
        if(m_cursorPos>0){
            if (isStringAtContent(m_cursorPos-1)) {
                if (getStringAtContent(m_cursorPos-1) == "\\") {
                    m_content.removeAt(m_cursorPos-1);
                    m_cursorPos--;
                    text = getGreekCharacter(event->text());
                }
            }
        }
        m_content.insert(m_cursorPos, text);
        m_cursorPos++;
        m_selectionEnd = m_selectionStart = -1;
        //updateBoundingRectAndLayout();
    }
    updateContStr();
//    qDebug() << "MathLeafEdit::insertText(QKeyEvent) END, m_contStr = " << m_contStr;
}

void MathLeafEdit::insertMathLeaf(MathLeafEdit* newLeaf) {
    updateContStr();
//    qDebug() << "MathLeafEdit::insertMathLeaf(MathLeafEdit) START, m_contStr = " << m_contStr;
    newLeaf->setParentItem(this);
    newLeaf->setParent(this);
    m_content.insert(m_cursorPos, QVariant::fromValue(newLeaf));
    m_cursorPos++;
    
    connect(newLeaf, &MathLeafEdit::itemSizeChanged, this, &MathLeafEdit::updateBoundingRectAndLayout);
    connect(newLeaf, &MathLeafEdit::gainedFocus, m_parentPageMathItem, &PageMathItem::onChildFocusIn);
    connect(newLeaf, &MathLeafEdit::lostFocus, m_parentPageMathItem, &PageMathItem::onChildFocusOut);
    connect(newLeaf, &MathLeafEdit::deleteMe, this, &MathLeafEdit::removeChild);
    connect(newLeaf, &MathLeafEdit::leftArrowAtStart, this, &MathLeafEdit::setCursorBeforeChild);
    connect(newLeaf, &MathLeafEdit::rightArrowAtEnd, this, &MathLeafEdit::setCursorAfterChild);
    
    //updateBoundingRectAndLayout();
    updateContStr();
//    qDebug() << "MathLeafEdit::insertMathLeaf(MathLeafEdit) END, m_contStr = " << m_contStr;
}
void MathLeafEdit::reconnectChildLeaf(MathLeafEdit *child, MathLeafEdit *dest) {
    updateContStr();
//    qDebug() << "MathLeafEdit::reconnectChildLeaf START, m_contStr = " << m_contStr;
    if (child && dest && child!=dest) {
        disconnect(child, &MathLeafEdit::itemSizeChanged, this, &MathLeafEdit::updateBoundingRectAndLayout);
        disconnect(child, &MathLeafEdit::gainedFocus, m_parentPageMathItem, &PageMathItem::onChildFocusIn);
        disconnect(child, &MathLeafEdit::lostFocus, m_parentPageMathItem, &PageMathItem::onChildFocusOut);
        disconnect(child, &MathLeafEdit::deleteMe, this, &MathLeafEdit::removeChild);
        disconnect(child, &MathLeafEdit::leftArrowAtStart, this, &MathLeafEdit::setCursorBeforeChild);
        disconnect(child, &MathLeafEdit::rightArrowAtEnd, this, &MathLeafEdit::setCursorAfterChild);
        
        MathLeafEdit* parentOfDest = qobject_cast<MathLeafEdit*>(dest->parentObject());
        if (parentOfDest) {
            connect(child, &MathLeafEdit::transferContentTo, parentOfDest, &MathLeafEdit::onTransferContentTo);
        }
    }
    updateContStr();
//    qDebug() << "MathLeafEdit::reconnectChildLeaf END, m_contStr = " << m_contStr;
}


void MathLeafEdit::backspace(bool childItemsAreMoved) {
    updateContStr();
//    qDebug() << "MathLeafEdit::backspace START, m_contStr = " << m_contStr << ", m_cursorPos = " << m_cursorPos;
    if (m_isFunction && m_content.count()<=1) {
        emit deleteMe(this);
    }
    int tempEnd = m_selectionEnd;
    if (m_selectionStart > -1 && m_selectionEnd > -1) m_cursorPos = m_selectionEnd;
    if (m_selectionStart == tempEnd) tempEnd += 1;
    for(int i=tempEnd; i>m_selectionStart; i--){
        if (m_cursorPos > 0) {
            m_cursorPos--;
            if (isStringAtContent(m_cursorPos)) {
                QString currentString = getStringAtContent(m_cursorPos);
                m_content.removeAt(m_cursorPos);
                if (isValidFunction(currentString)) {
                    m_isValidFunction = false;
                    m_isFunction = false;
                }
            } else if (isMathLeafEditAtContent(m_cursorPos)) {
                QGraphicsObject* child = getMathLeafEditAtContent(m_cursorPos);
                m_content.removeAt(m_cursorPos);
                if (!childItemsAreMoved) delete child;
            }
        }
    }
    //updateBoundingRectAndLayout();
    clearSelection();
    checkForParenthesis();
    checkForValidFunction();
    updateContStr();
//    qDebug() << "<<< MathLeafEdit::backspace END, m_contStr = " << m_contStr << ", m_cursorPos = " << m_cursorPos;
}

void MathLeafEdit::del() {updateContStr();
    if(m_selectionStart < m_selectionEnd) {
        backspace();
    } else {
        clearSelection();
        if(m_cursorPos < m_content.count()) {
            m_cursorPos += 1;
            backspace();
        }
    }
    return;
}

void MathLeafEdit::leftArrow() {updateContStr();
    if (m_cursorPos <= 0) { // Cursor at leftmost possible position, go to parent and set cursor before this child.
        emit leftArrowAtStart(this);
        return;
    } else {
        // check what kind of content is to the left:
        if (isStringAtContent(m_cursorPos-1)) {
            m_cursorPos--;
        } else if (isMathLeafEditAtContent(m_cursorPos-1)) {
            getMathLeafEditAtContent(m_cursorPos-1)->setCursorToLast();
        }
        //update();
    }
}
void MathLeafEdit::rightArrow() {updateContStr();
    if (m_cursorPos >= m_content.count()) { // Cursor at leftmost possible position, go to parent and set cursor before this child.
        emit rightArrowAtEnd(this);
        return;
    } else {
        // check what kind of content is to the left:
        if (isStringAtContent(m_cursorPos)) {
            m_cursorPos++;
        } else if (isMathLeafEditAtContent(m_cursorPos)) {
            getMathLeafEditAtContent(m_cursorPos)->setCursorToFirst();
        }
        //update();
    }
}

void MathLeafEdit::upArrow(MathLeafEdit *m) {updateContStr();
    if (m_isPiecewiseFunction) { // "{<OBJ>;<OBJ><OBJ>;<OBJ><OBJ>;<OBJ><OBJ>;<OBJ>"
        if (!(m==this)) {
            for (int i=0; i<m_content.size(); i++){
                if (isMathLeafEditAtContent(i)) {
                    MathLeafEdit *child = getMathLeafEditAtContent(i);
                    if (child == m) {
                        int line = (i-1)/3;
                        m_cursorPos = line * 3 + 1;
                    }
                }
            }
        }
        m_cursorPos = m_cursorPos - 3;
        if (m_cursorPos<4) m_cursorPos = 4;
        setFocus();
    } else {
        MathLeafEdit* parentMathLeafEdit = qobject_cast<MathLeafEdit*>(parentObject());
        if (parentMathLeafEdit) {
            if (parentMathLeafEdit->isPiecewiseFunction()) {
                parentMathLeafEdit->upArrow(m);
            }
        }
    }
}
void MathLeafEdit::downArrow(MathLeafEdit *m) {updateContStr();
    if (m_isPiecewiseFunction) { // "{<OBJ>;<OBJ><OBJ>;<OBJ><OBJ>;<OBJ><OBJ>;<OBJ>"
        if (!(m==this)) {
            for (int i=0; i<m_content.size(); i++){
                if (isMathLeafEditAtContent(i)) {
                    MathLeafEdit *child = getMathLeafEditAtContent(i);
                    if (child == m) {
                        int line = (i-1)/3;
                        m_cursorPos = line * 3 + 1;
                    }
                }
            }
        }
        m_cursorPos = m_cursorPos + 3;
        if (m_cursorPos>m_content.size()) m_cursorPos = m_content.size();
        setFocus();
    } else {
        MathLeafEdit* parentMathLeafEdit = qobject_cast<MathLeafEdit*>(parentObject());
        if (parentMathLeafEdit) {
            if (parentMathLeafEdit->isPiecewiseFunction()) {
                parentMathLeafEdit->downArrow(m);
            }
        }
    }
}

void MathLeafEdit::removeChild(MathLeafEdit *child) {
    if (!child) {
        qWarning() << "MathLeafEdit::removeChildLeaf: Attempted to remove a null child.";
    }
    // Iterate through content to find and remove the child
    for (int i = 0; i < m_content.size(); ++i) {
        if (isMathLeafEditAtContent(i)) {
            MathLeafEdit* currentChild = getMathLeafEditAtContent(i);
            if (currentChild == child) {
                m_content.removeAt(i);                   // Remove from the list
                setFocus();                              // Acquire focus
                m_cursorPos = qMin(i, m_content.size()); // Set cursor position
                child->deleteLater();                    // Mark for deletion at end of ebent loop
                //updateBoundingRectAndLayout();           // Update layout after removal
                return;
            }
        }
    }
    qWarning() << "MathLeafEdit::removeChildLeaf: Child not found in content list.";
}

void MathLeafEdit::setCursorBeforeChild(MathLeafEdit *child) {
    if (!child) {
        qWarning() << "MathLeafEdit::setCursorBeforeChild: Attempted to move a set cursor before a NULL child.";
    }
    // Iterate through content to find the child and set the cursor before the child
    for (int i = 0; i < m_content.size(); ++i) {
        if (isMathLeafEditAtContent(i)) {
            if (getMathLeafEditAtContent(i) == child) {
                if (m_isPiecewiseFunction) { // "{<OBJ>;<OBJ><OBJ>;<OBJ><OBJ>;<OBJ><OBJ>;<OBJ>"
                    int lineNumber = (i-1)/3;
                    int firstChildInLineID = lineNumber * 3 + 1;
                    MathLeafEdit *firstChild = getMathLeafEditAtContent(firstChildInLineID);
                    bool childIsFirstItemInLine = (firstChildInLineID == i);
                    firstChild->setFocus();
                    if (childIsFirstItemInLine) firstChild->setCursorToFirst();
                        else firstChild->setCursorToLast();
                } else {
                    setFocus();                              // Acquire focus
                    m_cursorPos = qMin(i, m_content.size()); // Set cursor position
                }
                return;
            }
        }
    }
    qWarning() << "MathLeafEdit::setCursorBeforeChild: Child not found in content list.";
}
void MathLeafEdit::setCursorAfterChild(MathLeafEdit *child) {
    if (!child) {
        qWarning() << "MathLeafEdit::setCursorAfterChild: Attempted to remove a null child.";
    }
    // Iterate through content to find the child and set the cursor after the child
    for (int i = 0; i < m_content.size(); ++i) {
        if (isMathLeafEditAtContent(i)) {
            if (getMathLeafEditAtContent(i) == child) {
                if (m_isPiecewiseFunction) { // "{<OBJ>;<OBJ><OBJ>;<OBJ><OBJ>;<OBJ><OBJ>;<OBJ>"
                    int lineNumber = (i-1)/3;
                    int lastChildInLineID = lineNumber * 3 + 3;
                    MathLeafEdit *lastChild = getMathLeafEditAtContent(lastChildInLineID);
                    bool childIsLastItemInLine = (lastChildInLineID == i);
                    if (childIsLastItemInLine) {
                        setFocus();
                        m_cursorPos = qMin(i+1, m_content.size());
                    } else {
                        lastChild->setFocus();
                        lastChild->setCursorToFirst();
                    }
                } else {
                    setFocus();                                // Acquire focus
                    m_cursorPos = qMin(i+1, m_content.size()); // Set cursor position
                }
                return;
            }
        }
    }
    qWarning() << "MathLeafEdit::setCursorAfterChild: Child not found in content list.";
}


bool MathLeafEdit::isEmpty() const {
    for (int i=0; i<m_content.size(); i++) {
        if (isStringAtContent(i)) {
            if (!getStringAtContent(i).isEmpty()) {
                return false;
            }
        } else if (isMathLeafEditAtContent(i)) {
            if (!getMathLeafEditAtContent(i)->isEmpty()) {
                return false;
            }
        }
    }
    return true;
}

bool MathLeafEdit::hasDescendantFocus() const {
    if (hasFocus()) {
        return true;
    }
    for (int i=0; i<m_content.size(); i++) {
        if (isMathLeafEditAtContent(i)) {
            if (getMathLeafEditAtContent(i)->hasDescendantFocus()) {
                return true;
            }
        }
    }
    return false;
}

void MathLeafEdit::setHighlighted(bool highlighted) {updateContStr();
    m_isHighlighted = highlighted;
    // Recursively set highlight state for children
    for (int i=0; i<m_content.size(); i++) {
        if (isMathLeafEditAtContent(i)) {
            getMathLeafEditAtContent(i)->setHighlighted(highlighted);
        }
    }
}

void MathLeafEdit::clearSelection() {updateContStr();
    m_selectionStart = m_selectionEnd = -1;
    // Also clear highlighting on children
    for (int i=0; i<m_content.size(); i++) {
        if (isMathLeafEditAtContent(i)) {
            getMathLeafEditAtContent(i)->setHighlighted(false);
        }
    }
    update();
}

void MathLeafEdit::cleanContent() {updateContStr();
    m_selectionStart = m_selectionEnd = -1;
    for (int i=0; i<m_content.size(); i++) {
        QVariant item = m_content[i];
        if (isStringAtContent(i)) {
            QString str = getStringAtContent(i);
            if (str == "\\" || str == "ß" || str.isEmpty()) {
                m_content.removeAt(i);updateContStr();
                m_cursorPos--;
            }
        }
        else if (isMathLeafEditAtContent(i)) {
            if (MathLeafEdit* childLeaf = getMathLeafEditAtContent(i)) {
                if (!childLeaf->isEmpty()) {
                    childLeaf->cleanContent();
                }
            }
        }
    }
}

void MathLeafEdit::keyPressEvent(QKeyEvent* event) {updateContStr();
//    qDebug() << "MathLeafEdit::keyPressEvent: Key ID = " << event->key() << " ,  Key text = '"<< event->text() <<"'";
    printContent();
    const bool altPressed = event->modifiers() & Qt::AltModifier;
    const bool ctrlPressed = event->modifiers() & Qt::ControlModifier;
    
    QString charToInsert = event->text();
    if (event->key() == Qt::Key_AsciiCircum || event->key() == Qt::Key_Dead_Circumflex) charToInsert = "^";
    
    if (charToInsert.length() == 1){
        QChar qc = charToInsert.at(0);
        if(!m_isFunction && !m_isPiecewiseFunction) {
            if (qc.isLetterOrNumber() ||                                          // 0x00B0 = '°'
                (qc == '\\') || (qc == '.') || (qc == ',') || (qc == '_') || (qc == degChar)) {
                if ( !couldBeNumber(currentTokenAll()) ) {
                    insertText(event);
                    QString token = currentTokenAll(); // Make a QString from cursor backwards until end of the token
                    if ( couldBeNumber(token) ) backspace();
                }
            }
            if ((qc >= '0'  &&  qc <= '9') || (qc == '+') || (qc == '-') ||
                (qc == 'E') || (qc == 'e') || (qc == '.') || (qc == ',')) {
                insertText(event);
                QString token = currentTokenAll(); // Make a QString from cursor backwards until end of the token
                if ( !couldBeNumber(token) ) {
                    backspace();
                    if ( (qc == '+') || (qc == '-') ) {
                        insertMathLeaf(new MathLeafEdit(qc, m_data, m_parentPageMathItem, this));
                        setFocus(); // get the focus back from newly created MathLeafEdit object
                    }
                }
            }
            if (qc == '*' || qc == ' '){
                insertMathLeaf(new MathLeafEdit(charToInsert, m_data, m_parentPageMathItem, this));
                setFocus(); // get the focus back from newly created MathLeafEdit object
            }
            if (qc == '<'){
                QString qcStr; if (altPressed) qcStr = smallerEqualChar; else qcStr = "<";
                insertMathLeaf(new MathLeafEdit(qcStr, m_data, m_parentPageMathItem, this));
                setFocus(); // get the focus back from newly created MathLeafEdit object
            }
            if (qc == '>'){
                QString qcStr; if (altPressed) qcStr = greaterEqualChar; else qcStr = ">";
                insertMathLeaf(new MathLeafEdit(qcStr, m_data, m_parentPageMathItem, this));
                setFocus(); // get the focus back from newly created MathLeafEdit object
            }
            if (qc == '&'){
                insertMathLeaf(new MathLeafEdit(charToInsert, m_data, m_parentPageMathItem, this));
                setFocus(); // get the focus back from newly created MathLeafEdit object
            }
            if (qc == '|'){
                insertMathLeaf(new MathLeafEdit(charToInsert, m_data, m_parentPageMathItem, this));
                setFocus(); // get the focus back from newly created MathLeafEdit object
            }
            if (qc == '/'){
                MathLeafEdit *quotient = new MathLeafEdit(charToInsert, m_data, m_parentPageMathItem, this);
                //if (m_persistSelectionEnd>-1) m_cursorPos = m_persistSelectionEnd;
                insertMathLeaf(quotient);
            }
            if (qc == ':'){
                insertMathLeaf(new MathLeafEdit(charToInsert, m_data, m_parentPageMathItem, this));
            }
            if (qc == '='){ // ALT+"=" is "equality", "=" is "display result"
                QString qcStr; if (altPressed) qcStr = longEqualChar; else qcStr = "=";
                insertMathLeaf(new MathLeafEdit(qcStr, m_data, m_parentPageMathItem, this));
                if (altPressed) setFocus();
            }
            if (qc == '^'){
                MathLeafEdit *m = new MathLeafEdit(charToInsert, m_data, m_parentPageMathItem, this);
                insertMathLeaf(m);
            }
            if (qc == '('){
                m_persistSelectionStart = m_selectionStart; m_persistSelectionEnd = m_selectionEnd;
                insertText(event); m_showParenthesis = true;
                QString token = currentTokenBackwards();
                //bool isParenthesisOnly = (token == "(");
                if (isValidFunction(token)) {
                    MathLeafEdit *fContent = nullptr;
                    if (m_cursorPos<m_content.size()) { // check if cursor is not at last position
                        if (isMathLeafEditAtContent(m_cursorPos)) { // check what is the item after the cursor
                            fContent = getMathLeafEditAtContent(m_cursorPos);
                            if (fContent->isFunctionIsSet() && !fContent->isValidFunctionIsSet()) { // must be +, -, *, / but not sin( ...
                                fContent = nullptr; // for +, -, * set no content right of cursor
                            }
                        }
                    }
                    m_selectionEnd = m_cursorPos;
                    m_selectionStart = m_cursorPos - token.size();
                    backspace();
                    if (fContent) {
                        insertText(token);
                        m_isFunction = m_isValidFunction = m_showParenthesis = true; 
                    } else {
                        if(m_persistSelectionStart != m_persistSelectionEnd) m_cursorPos = m_persistSelectionStart;
                        m_selectionStart = m_persistSelectionStart; m_selectionEnd = m_persistSelectionEnd;
                        insertMathLeaf(new MathLeafEdit(token, m_data, m_parentPageMathItem, this));
                    }
                }
            }
            if (qc == '{'){
                m_persistSelectionStart = m_selectionStart = m_persistSelectionEnd = m_selectionEnd = -1;
                insertMathLeaf(new MathLeafEdit(charToInsert, m_data, m_parentPageMathItem, this));
            }
        } else {
            updateContStr();
            if (qc == '*') qc = cdotChar; // replace * with dot
            if (qc == '=' && altPressed) qc = longEqualChar; // replace = with ==
            if (qc == '<' && altPressed) qc = smallerEqualChar; // replace < with <=
            if (qc == '>' && altPressed) qc = greaterEqualChar; // replace > with >=
            if ((qc == '+' || qc == '-' || qc == cdotChar || qc == ' ') &&
                   (m_content.at(0) == "+" || m_content.at(0) == "-" ||
                    m_content.at(0) == cdotChar || m_content.at(0) == " " ||
                    m_content.at(0) == "<" || m_content.at(0) == ">" ||
                    m_content.at(0) == "&" || m_content.at(0) == "|" ||
                    m_content.at(0) == smallerEqualChar || m_content.at(0) == greaterEqualChar ||
                    m_content.at(0) == longEqualChar ||
                    m_content.at(0) == ":=" || m_content.at(0) == "=")) {
                m_content[0] = QString(qc);
                if (qc == '+' || qc == '-') m_paddingH = 3; else m_paddingH = 0;
            } else if ((qc == ':') &&
                  (m_content.at(0) == "+" || m_content.at(0) == "-" ||
                   m_content.at(0) == cdotChar || m_content.at(0) == " " ||
                   m_content.at(0) == "<" || m_content.at(0) == ">" ||
                   m_content.at(0) == "&" || m_content.at(0) == "|" ||
                   m_content.at(0) == smallerEqualChar || m_content.at(0) == greaterEqualChar ||
                   m_content.at(0) == longEqualChar)) {
                m_content[0] = QString(":=");
            } else {
                checkForValidFunction();
            }
        }
        emit m_parentPageMathItem->itemDataChanged();
    }
    
    if (event->key() == Qt::Key_Backspace) {
        backspace();
        emit m_parentPageMathItem->itemDataChanged();
    } else if (event->key() == Qt::Key_Delete) {
        del();
        emit m_parentPageMathItem->itemDataChanged();
    } else if (event->key() == Qt::Key_Left) {
        leftArrow();
    } else if (event->key() == Qt::Key_Right) {
        rightArrow();
    } else if (event->key() == Qt::Key_Up) {
        upArrow(this);
    } else if (event->key() == Qt::Key_Down) {
        downArrow(this);
    } else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        if (m_isPiecewiseFunction) {
            MathLeafEdit* newBool = new MathLeafEdit(m_data, m_parentPageMathItem, this);
            MathLeafEdit* newFunc = new MathLeafEdit(m_data, m_parentPageMathItem, this);
            insertMathLeaf(newBool);
            insertText(";");
            insertMathLeaf(newFunc);
            updateContStr();
            newBool->updateBoundingRectAndLayout();
            newFunc->updateBoundingRectAndLayout();
            newBool->setFocus();
        }
    } else {
        if (!(event->modifiers() & Qt::AltModifier)) {
            QGraphicsObject::keyPressEvent(event);
        }
    }
    //m_persistSelectionStart = m_selectionStart;
    //m_persistSelectionEnd = m_selectionEnd;
    checkForParenthesis();
    event->accept();
    updateBoundingRectAndLayout();
    printContent();
}

void MathLeafEdit::mousePressEvent(QGraphicsSceneMouseEvent* event) {updateContStr();
    if(hasFocus()) {
        event->accept();
        grabMouse(); // "this" now gets all move events until release
        clearSelection(); // Clear any previous selection
        m_selectionStart = m_selectionEnd = m_selectionAnchor = getCursorIndexForPosition(event->pos().x());
        m_cursorPos = m_selectionStart;
        update(m_boundingRectangle);
        m_persistSelectionStart = m_selectionStart;
        m_persistSelectionEnd = m_selectionEnd;
    } else {
        QGraphicsObject::mousePressEvent(event);
    }
}
void MathLeafEdit::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (scene() && scene()->mouseGrabberItem() == this) {
        ungrabMouse();
    }
    QGraphicsObject::mouseReleaseEvent(event);
}

void MathLeafEdit::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {updateContStr();
    if ((event->buttons() & Qt::LeftButton) && m_selectionAnchor>-1) {
        int mousePos = getCursorIndexForPosition(event->pos().x());
        m_selectionStart = std::min(m_selectionAnchor, mousePos);
        m_selectionEnd   = std::max(m_selectionAnchor, mousePos);
        event->accept();
        update(m_boundingRectangle);
        m_persistSelectionStart = m_selectionStart;
        m_persistSelectionEnd = m_selectionEnd;
    }
    QGraphicsObject::mouseMoveEvent(event);
}

// Helper function to determine and set the appropriate cursor
void MathLeafEdit::updateHoverCursor(const QPointF &pos) {updateContStr();
    if (m_boundingRectangle.contains(pos)) { // Mouse is over the text content area: indicate editable with Qt::IBeamCursor
        setCursor(QCursor(Qt::IBeamCursor));
    }
}

// Override hoverEnterEvent to set the cursor when mouse enters the item's area
void MathLeafEdit::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {updateContStr();
    updateHoverCursor(event->pos());
    QGraphicsObject::hoverEnterEvent(event);
}

// Override hoverLeaveEvent to reset the cursor when mouse leaves the item's area
void MathLeafEdit::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {updateContStr();
    unsetCursor(); // Reset cursor to the default scene cursor
    QGraphicsObject::hoverLeaveEvent(event);
}

void MathLeafEdit::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {updateContStr();
    updateHoverCursor(event->pos());
    QGraphicsObject::hoverMoveEvent(event);
}

void MathLeafEdit::focusInEvent(QFocusEvent* event) {updateContStr();
    m_cursorTimer.start(500);
    m_cursorVisible = true;
    update(m_boundingRectangle);
    emit gainedFocus();
    QGraphicsObject::focusInEvent(event);
}

void MathLeafEdit::focusOutEvent(QFocusEvent* event) {updateContStr();
    m_cursorTimer.stop();
    m_cursorVisible = false;
    clearSelection();
    update(m_boundingRectangle);
    emit lostFocus();
    QGraphicsObject::focusOutEvent(event);
}

void MathLeafEdit::toggleCursor() {updateContStr();
    m_cursorVisible = !m_cursorVisible;
    QRectF cursorRect = getCursorRect(); // in item coordinates
    update(cursorRect);
}

void MathLeafEdit::updateBoundingRectAndLayout() {
    updateContStr();
    prepareGeometryChange();
    
    m_boundingRectangle = updateBoundingRect();
    
    qreal currentWidth = 1.0*m_paddingH;
    
    QFontMetricsF fm(m_font);
    qreal maxHeight = fm.height();
    QFont subscriptFont(m_fontName, m_mathFontSize*m_subScriptScale);
    //QFont subscriptFont(m_fontName, fm.height()*m_subScriptScale);
    QFontMetricsF subFm(subscriptFont);
    bool subscriptActive = false;
    
    if ( m_isPiecewiseFunction && m_showBraces ) {
        //m_contStr == "{<OBJ>;<OBJ>"
        qreal lWidthMax = 0.0;
        qreal lHeightMax = 0.0;
        for (int i=0; i<m_content.size(); i++) {
            if (isStringAtContent(i)) {
                if ( getStringAtContent(i) == "{" ) {
                    currentWidth += fm.horizontalAdvance(getStringAtContent(i));
                }
                if ( getStringAtContent(i) == ";" ) {
                    MathLeafEdit* mleBool = getMathLeafEditAtContent(i-1);
                    MathLeafEdit* mleFunc = getMathLeafEditAtContent(i+1);
                    qreal lWidth = currentWidth + mleBool->boundingRectangle().width() + mleFunc->boundingRectangle().width();
                    qreal lHeight = qMax(mleBool->boundingRectangle().height(),
                                         mleFunc->boundingRectangle().height());
                    lWidthMax = qMax(lWidthMax, lWidth);
                    lHeightMax += lHeight;
                }
            }
        }
        currentWidth = lWidthMax; maxHeight = lHeightMax;
        updateBracesPath(QPointF(0.0,0.0), lHeightMax, true);
        
    } else {
        for (int i=0; i<m_content.size(); i++) {
            if (isStringAtContent(i)) {
                QString text = getStringAtContent(i);
                if (text == "_" && !subscriptActive) { subscriptActive = true; continue;
                } else {
                    if (subscriptActive) { currentWidth += subFm.horizontalAdvance(text);
                    } else { currentWidth += fm.horizontalAdvance(text); }
                    if ( m_isValidFunction && m_isFunction && m_showParenthesis ) { // Draw a ( ) around the content of the function
                        if (i<m_content.size()-1) { if (isMathLeafEditAtContent(i+1)) {
                            MathLeafEdit* fContent = getMathLeafEditAtContent(i+1);
                            if (m_contStr == "log(<OBJ><OBJ>") fContent = getMathLeafEditAtContent(2);
                            qreal h = fContent->boundingRect().height();
                            qreal d = fm.horizontalAdvance("(");
                            updateParenthesisPath(QPointF(currentWidth-d-0.2*m_paddingH,0), h, true);
                            //currentWidth += d;
                        }}
                    }
                }
            } else if (isMathLeafEditAtContent(i)) {
                subscriptActive = false;
                MathLeafEdit* child = getMathLeafEditAtContent(i);
                currentWidth += child->boundingRect().width() + m_paddingH/3.0;
                maxHeight = qMax(maxHeight, child->boundingRect().height() + 2.0*m_paddingV);
                if ( m_isValidFunction && m_isFunction && m_showParenthesis) { // Draw a ( ) around the content of the function
                    if (i>0){
                        updateParenthesisPath(QPointF(currentWidth,0), child->boundingRect().height(), false);
                        currentWidth += fm.horizontalAdvance(")");
                    }
                }
                
            }
        }
    }
    
    if (m_contStr.isEmpty()) { currentWidth = 10; }
    m_textBoundingRect = QRectF(0, 0, currentWidth, maxHeight);
    
    updateChildPositions();
    emit itemSizeChanged(); // Signal to parent, that this itemsize has changed
    update();
}

void MathLeafEdit::updateChildPositions() {
    //prepareGeometryChange();
    updateContStr();
    checkChildrenMaxCenterHeight();
    qreal currentX = m_paddingH;
    QFontMetricsF fm(m_font);
    QFont subscriptFont(m_fontName, m_mathFontSize*m_subScriptScale);
    //QFont subscriptFont(m_fontName, fm.height()*m_subScriptScale);
    QFontMetricsF subFm(subscriptFont);
    bool subscriptActive = false;
    if (m_contStr==QString("<OBJ>/<OBJ>")) { // This is a division (fraction)
        MathLeafEdit* numer = getMathLeafEditAtContent(0);
        MathLeafEdit* denom = getMathLeafEditAtContent(2);
        if (numer && denom) {
            qreal numW = numer->boundingRect().width();
            qreal numH = numer->m_boundingRectangle.height();
            qreal denW = denom->boundingRect().width();
            qreal denH = denom->m_boundingRectangle.height();
            qreal maxW = qMax(numW, denW);
            qreal numX = 0.5*(maxW - numW);
            qreal denX = 0.5*(maxW - denW);
            //qreal numY = - numH-2*m_paddingV + 0.5*boundingRect().height();
            //qreal denY = 2*m_paddingV + 0.5*boundingRect().height();
            qreal numY = 0.0;
            qreal denY = numH + 3.5*m_paddingV;
            numer->setPos(numX, numY); denom->setPos(denX, denY);
            m_centerHeight = numH + 0.0+m_paddingV;
            //currentX += m_paddingH/3.0 + maxW;
        }
    } else if (m_contStr==QString("<OBJ>^<OBJ>")) { // This is an exponent (power)
        MathLeafEdit* base = getMathLeafEditAtContent(0);
        MathLeafEdit* expo = getMathLeafEditAtContent(2);
        if (base && expo) {
            qreal baseW = base->boundingRect().width();
            qreal baseH = base->m_boundingRectangle.height();
            qreal expoW = expo->boundingRect().width();
            qreal expoH = expo->m_boundingRectangle.height();
            qreal baseX = 0.0; qreal baseY = qMax(expoH - 0.4*baseH, 0.5*expoH);
            qreal expoX = baseW; qreal expoY = 0.0;
            base->setPos(baseX, baseY); expo->setPos(expoX, expoY);
            m_centerHeight = baseY+base->centerHeight()+m_paddingV;
            //currentX += m_paddingH/3.0 + maxW;
        }
    } else if (m_contStr==QString("root(<OBJ><OBJ>")) { // This is a root
        MathLeafEdit* radical = getMathLeafEditAtContent(1);
        MathLeafEdit* radicand = getMathLeafEditAtContent(2);
        if (radical && radicand) {
            qreal radicalW = radical->boundingRect().width();
            qreal radicalH = radical->boundingRectangle().height();
            qreal radicandW = radicand->boundingRect().width();
            qreal radicandH = radicand->boundingRectangle().height();
            
            qreal inc = 1.0*fm.horizontalAdvance("_");
            // Position of base and exponent rectangles (must be same as in updateChildPositions() )
            qreal radicalX = 0.0; qreal radicalY = 0.0;
            qreal radicandX = radicalW + inc; qreal radicandY = 2*m_paddingV;
            
            radical->setPos(radicalX, radicalY); radicand->setPos(radicandX, radicandY);
            m_centerHeight = radicandY+radicand->centerHeight()+m_paddingV;
        }
    } else if(m_contStr == "log(<OBJ><OBJ>" && m_isFunction) {
        MathLeafEdit* base = getMathLeafEditAtContent(1);
        MathLeafEdit* arg = getMathLeafEditAtContent(2);
        if (base && arg) {
            qreal baseW = base->boundingRect().width();
            qreal baseH = base->boundingRectangle().height();
            qreal argW = arg->boundingRect().width();
            qreal argH = arg->boundingRectangle().height();
            m_centerHeight = arg->centerHeight()+m_paddingV;
            
            // Position of base and exponent rectangles (must be same as in updateChildPositions() )
            qreal baseX = fm.horizontalAdvance("log")+m_paddingH; qreal baseY = m_centerHeight-0.5*m_paddingV;
            qreal argX = baseX+baseW + fm.horizontalAdvance(" ("); qreal argY = m_paddingV;
            
            base->setPos(baseX, baseY); arg->setPos(argX, argY);
        }
    } else if(m_contStr == "mod(<OBJ><OBJ>" && m_isFunction) {
        MathLeafEdit* numer = getMathLeafEditAtContent(1);
        MathLeafEdit* denom = getMathLeafEditAtContent(2);
        if (numer && denom) {
            qreal numerW = numer->boundingRect().width();
            qreal numerH = numer->boundingRectangle().height();
            qreal denomW = denom->boundingRect().width();
            qreal denomH = denom->boundingRectangle().height();
            m_centerHeight = qMax(numer->centerHeight(), denom->centerHeight())+m_paddingV;
            
            // Position of numer and denom rectangles (must be same as in updateChildPositions() )
            qreal numerX = m_paddingH; qreal numerY = m_centerHeight-0.5*numerH-m_paddingV;
            qreal denomX = numerX + m_paddingH; qreal denomY = m_centerHeight-0.5*denomH-m_paddingV;
            
            numer->setPos(numerX, numerY); denom->setPos(denomX, denomY);
        }
    } else if (m_isPiecewiseFunction) { // This is a piecewise defined function
        QList < QPair<MathLeafEdit*, MathLeafEdit*> > pieceWiseFunction;
        for (int i = 0; i<(m_content.size()-1)/3; i++) {
            pieceWiseFunction.append( QPair<MathLeafEdit*, MathLeafEdit*>(getMathLeafEditAtContent(i*3+1), getMathLeafEditAtContent(i*3+3)) );
        }
        qreal x_0 = 1.0*fm.horizontalAdvance("{");
        qreal y = 0.0;
        for (int i = 0; i<pieceWiseFunction.size(); i++) {
            MathLeafEdit* cond = pieceWiseFunction[i].first;
            MathLeafEdit* func = pieceWiseFunction[i].second;
            qreal condW = cond->boundingRect().width();
            qreal condH = cond->m_boundingRectangle.height();
            qreal funcW = func->boundingRect().width();
            qreal funcH = func->m_boundingRectangle.height();
            qreal bothMax = qMax(condH, funcH);
            
            qreal condOffset = 0, funcOffset = 0;
            if (condH > funcH) funcOffset = (condH - funcH) * 0.5;
                else condOffset = (funcH - condH) * 0.5;
            
            qreal condX = x_0; qreal condY = y + condOffset;
            qreal funcX = condX + condW + fm.horizontalAdvance("; "); qreal funcY = y + funcOffset;
            cond->setPos(condX, condY); func->setPos(funcX, funcY);
            y += bothMax;
        }
        m_centerHeight = y/2.0 + m_paddingV;
        //currentX += m_paddingH/3.0 + maxW;
    } else {
        for (int i=0; i<m_content.size(); i++) {
            if (isMathLeafEditAtContent(i)) {
                subscriptActive = false;
                MathLeafEdit* child = getMathLeafEditAtContent(i);
                qreal childY = m_centerHeight - child->centerHeight() - 0*m_paddingV;
                
                child->setPos(currentX, childY);
                currentX += child->boundingRect().width() + m_paddingH/3.0;
            } else if (isStringAtContent(i)) {
                QString text = getStringAtContent(i);
                if (text == "_" && !subscriptActive) { subscriptActive = true; continue;
                } else {
                    if (subscriptActive) { currentX += subFm.horizontalAdvance(text);
                    } else { currentX += fm.horizontalAdvance(text); }
                }
            }
        }
    }
}

int MathLeafEdit::getCursorIndexForPosition(qreal x) {updateContStr();
    qreal currentX = 0.0;
    QFontMetricsF fm(m_font);
    QFont subscriptFont(m_fontName, m_mathFontSize*m_subScriptScale);
    //QFont subscriptFont(m_fontName, fm.height()*m_subScriptScale);
    QFontMetricsF subFm(subscriptFont);
    bool subscriptActive = false;
    
    for (int i = 0; i < m_content.size(); ++i) {
        qreal itemWidth = 0.0;
        if (isStringAtContent(i)) {
            QString text = getStringAtContent(i);
            if (text == "_" && !subscriptActive) {
                subscriptActive = true;
                itemWidth = 0.0;
            } else {
                if (subscriptActive) { itemWidth = subFm.horizontalAdvance(text);
                } else { itemWidth = fm.horizontalAdvance(text); }
            }
        } else if (isMathLeafEditAtContent(i)) {
            subscriptActive = false;
            itemWidth = getMathLeafEditAtContent(i)->m_boundingRectangle.width();
        }
        
        // If the click is within this item's horizontal bounds, set cursor here
        if (x >= currentX && x < currentX + itemWidth) {
            // For string items, we need to find the closest character
            if (isStringAtContent(i)) {
                if(x <= currentX + itemWidth /2.0) return i;
                else return i+1;
            }
            return i; // For non-string items, place cursor before the item
        }
        currentX += itemWidth;
    }
    
    // If click is past all content, place cursor at the end
    return m_content.size();
}

void MathLeafEdit::printContent() {updateContStr();
    QString s("");
    for(int i=0; i<m_content.size(); i++){
        if (isStringAtContent(i)) {
            s = s + getStringAtContent(i);
        } else if (isMathLeafEditAtContent(i)) {
            s = s + QString("<OBJ>");
        }
    }
    qDebug() << "---> m_content = " << s << ", m_cursorPos = " << m_cursorPos
    << "\n     m_selectionStart = " << m_selectionStart << ", m_selectionEnd = " << m_selectionEnd;
}
void MathLeafEdit::updateContStr() const{
    m_contStr = "";
    for(int i=0; i<m_content.size(); i++){
        if (isStringAtContent(i)) {
            QString str = getStringAtContent(i);
            if (str.isEmpty()) 
                m_contStr = m_contStr + QString("<EMPTY>");
            else
                m_contStr = m_contStr + str;
        } else if (isMathLeafEditAtContent(i)) {
            m_contStr = m_contStr + QString("<OBJ>");
        }
    }
}

QString MathLeafEdit::currentTokenBackwards() {
    QString tokenStr = "";
    if (m_cursorPos > 0) {
        for(int i = m_cursorPos-1; i>=0; i--) {
            if (isStringAtContent(i)) {
                QString str = getStringAtContent(i);
                if (!str.isEmpty()) 
                    tokenStr = str + tokenStr;
            } else {
                break;
            }
        }
    }
    return tokenStr;
}
QString MathLeafEdit::currentTokenAll() {
    QString tokenStr = "";
    if (m_cursorPos > 0) {
        for(int i = m_cursorPos-1; i>=0; i--) {
            if (isStringAtContent(i)) {
                QString str = getStringAtContent(i);
                if (!str.isEmpty()) 
                    tokenStr = str + tokenStr;
            } else { break; }
        }
    }
    if (m_cursorPos < m_content.count()) {
        for(int i = m_cursorPos; i < m_content.count(); i++) {
            if (isStringAtContent(i)) {
                QString str = getStringAtContent(i);
                if (!str.isEmpty()) 
                    tokenStr = tokenStr + str;
            } else { break; }
        }
    }
    return tokenStr;
}
QString MathLeafEdit::tokenAllAt(int pos) {
    QString tokenStr = "";
    if (pos > 0) {
        for(int i = pos-1; i>=0; i--) {
            if (isStringAtContent(i)) {
                QString str = getStringAtContent(i);
                if (!str.isEmpty()) 
                    tokenStr = str + tokenStr;
            } else { break; }
        }
    }
    if (pos < m_content.count()) {
        for(int i = pos; i < m_content.count(); i++) {
            if (isStringAtContent(i)) {
                QString str = getStringAtContent(i);
                if (!str.isEmpty()) 
                    tokenStr = tokenStr + str;
            } else { break; }
        }
    }
    return tokenStr;
}

void MathLeafEdit::handleSimulatedKeyPress(int key, const QString& text)
{
    QKeyEvent* simulatedEvent = new QKeyEvent(QEvent::KeyPress, key, Qt::NoModifier, text);
    QCoreApplication::postEvent(this, simulatedEvent);
    
    //qDebug() << "MathLeafEdit::handleSimulatedKeyPress: Posted simulated key event for Key ID =" << key;
}

MathVariable MathLeafEdit::getValue() {
    return m_value;
}

void MathLeafEdit::checkForValidFunction() {
    qreal cursorPos = m_cursorPos;
    m_isValidFunction = false;
    for(int i=1; i<=m_content.size(); i++) {
        m_cursorPos = i;
        if (isStringAtContent(i-1)) {
            if(getValidFunctions().contains(currentTokenAll())) m_isValidFunction = true;
            break;
        }
    }
    m_cursorPos = cursorPos;
}

void MathLeafEdit::checkForParenthesis() {
    for (int i = 0; i < m_content.size(); ++i) {
        if (isStringAtContent(i)) {
            if (getStringAtContent(i).contains("(")) {
                m_showParenthesis = true;
                return;
            }
        }
    }
    m_showParenthesis = false;
    m_leftParenthesis->hide(); m_rightParenthesis->hide();
    return;
}

bool MathLeafEdit::isStringAtContent(int i) {
    if ( i<0 || i>=m_content.size() ) return false;
    if (m_content.at(i).metaType().id() == QMetaType::QString) return true;
    return false;
}
bool MathLeafEdit::isString(const QVariant &v) {
    return v.canConvert<QString>();
}
bool MathLeafEdit::isMathLeafEditAtContent(int i) {
    if ( i<0 || i>=m_content.size() ) return false;
    if (m_content.at(i).canConvert<MathLeafEdit*>()) return true;
    return false;
}
bool MathLeafEdit::isMathLeafEdit(const QVariant &v) {
    return v.canConvert<MathLeafEdit*>();
}

bool MathLeafEdit::isStringAtContent(int i) const {
    if ( i<0 || i>=m_content.size() ) return false;
    if (m_content.at(i).metaType().id() == QMetaType::QString) return true;
    return false;
}
bool MathLeafEdit::isString(const QVariant &v) const {
    return v.canConvert<QString>();
}
bool MathLeafEdit::isMathLeafEditAtContent(int i) const {
    if ( i<0 || i>=m_content.size() ) return false;
    if (m_content.at(i).canConvert<MathLeafEdit*>()) return true;
    return false;
}
bool MathLeafEdit::isMathLeafEdit(const QVariant &v) const {
    return v.canConvert<MathLeafEdit*>();
}


QString MathLeafEdit::getStringAtContent(int i) {
    if ( i<0 || i>=m_content.size() ) return QString();
    QVariant& content = m_content[i];
    if (content.metaType().id() == QMetaType::QString) {
        QString str = QString(content.toString());
        return str;
    }
    return QString();
}
QString MathLeafEdit::getString(const QVariant &v) {
    if (isString(v)) {
        QString str = QString(v.toString());
        return str;
    }
    return QString();
}
MathLeafEdit* MathLeafEdit::getMathLeafEditAtContent(int i) {
    if ( i<0 || i>=m_content.size() ) return nullptr;
    if (m_content.at(i).canConvert<MathLeafEdit*>()) {
        MathLeafEdit* fContent = qgraphicsitem_cast<MathLeafEdit*>(m_content.at(i).value<QGraphicsObject*>());
        return fContent;
    }
    return nullptr;
}
MathLeafEdit* MathLeafEdit::getMathLeafEdit(const QVariant &v) {
    if (isMathLeafEdit(v)) {
        MathLeafEdit* fContent = qgraphicsitem_cast<MathLeafEdit*>(v.value<QGraphicsObject*>());
        return fContent;
    }
    return nullptr;
}

QString MathLeafEdit::getStringAtContent(int i) const {
    if ( i<0 || i>=m_content.size() ) return QString();
    const QVariant& content = m_content.at(i);
    if (content.metaType().id() == QMetaType::QString) {
        QString str = QString(content.toString());
        return str;
    }
    return QString();
}
QString MathLeafEdit::getString(const QVariant &v) const {
    if (isString(v)) {
        QString str = QString(v.toString());
        return str;
    }
    return QString();
}
MathLeafEdit* MathLeafEdit::getMathLeafEditAtContent(int i) const {
    if ( i<0 || i>=m_content.size() ) return nullptr;
    if (m_content.at(i).canConvert<MathLeafEdit*>()) {
        MathLeafEdit* fContent = qgraphicsitem_cast<MathLeafEdit*>(m_content.at(i).value<QGraphicsObject*>());
        return fContent;
    }
    return nullptr;
}
MathLeafEdit* MathLeafEdit::getMathLeafEdit(const QVariant &v) const {
    if (isMathLeafEdit(v)) {
        MathLeafEdit* fContent = qgraphicsitem_cast<MathLeafEdit*>(v.value<QGraphicsObject*>());
        return fContent;
    }
    return nullptr;
}

void MathLeafEdit::onTransferContentTo(MathLeafEdit *me) {
    if (m_persistSelectionStart == m_persistSelectionEnd) { // nothing is selected, get the token near the cursor
        int cursorPos = m_persistSelectionStart;
        if (m_cursorPos > 0) {
            if (isStringAtContent(m_cursorPos -1)) {
                QString token = currentTokenBackwards();
                for (int i=0; i<token.size(); i++) {// with this we run through the token string
                    me->insertText(token.at(i));
                }
                m_selectionStart = m_cursorPos - token.size(); m_selectionEnd = m_cursorPos;
                backspace();
            } else if (isMathLeafEditAtContent(m_cursorPos -1)) {
                MathLeafEdit *child = getMathLeafEditAtContent(m_cursorPos -1);
                if (!child->isBasicOperator()) {
                    reconnectChildLeaf(child, me);
                    me->insertMathLeaf(child);
                    backspace(true); // we want to MOVE the child items, not delete them from memory
                }
            }
        }
    } else { // if something is selected, get the whole selection
        m_selectionStart = m_persistSelectionStart; 
        m_selectionEnd = m_persistSelectionEnd;
        m_cursorPos = m_selectionEnd;
        for (int i=m_persistSelectionStart; i<m_persistSelectionEnd; i++) {
            if (isStringAtContent(i)) {
                me->insertText(getStringAtContent(i));
            } else if (isMathLeafEditAtContent(i)) {
                MathLeafEdit *child = getMathLeafEditAtContent(i);
                reconnectChildLeaf(child, me);
                me->insertMathLeaf(child);
            }
        }
        backspace(true); // we want to MOVE the child items, not delete them from memory
    }
}

void MathLeafEdit::updateParenthesisPath(const QPointF& topLeft, qreal totalHeight, bool isLeftParenthesis) {
    //qDebug() << "MathLeafEdit::updateParenthesisPath";
    qreal stretchBy = totalHeight -20;
    if (stretchBy < 0.0) stretchBy = 0.0;
    qreal dx = topLeft.x();
    qreal dy = topLeft.y();
    QPointF Point1, cPoint1_1, cPoint1_2, Point2;
    QPointF Point3, cPoint3_1, cPoint3_2, Point4;
    QPointF Point5, cPoint5_1, cPoint5_2, Point6;
    QPointF Point7, cPoint7_1, cPoint7_2, Point8;
    if (isLeftParenthesis) {
        QPainterPath leftPath, leftLinePath;
        Point1    = QPointF(5.8+dx,  0.0+dy);	
        cPoint1_1 = QPointF(2.3+dx,  1.8+dy);
        cPoint1_2 = QPointF(0.0+dx,  4.3+dy);
        Point2    = QPointF(0.0+dx,  9.5+dy);
        
        Point3    = QPointF(0.0+dx, 10.5+dy + stretchBy);	
        cPoint3_1 = QPointF(0.0+dx, 15.7+dy + stretchBy);
        cPoint3_2 = QPointF(2.3+dx, 18.2+dy + stretchBy);
        Point4    = QPointF(5.8+dx, 20.0+dy + stretchBy);
        
        Point5    = QPointF(5.8+dx, 19.0+dy + stretchBy);	
        cPoint5_1 = QPointF(2.8+dx, 17.3+dy + stretchBy);
        cPoint5_2 = QPointF(2.1+dx, 13.2+dy + stretchBy);
        Point6    = QPointF(2.1+dx, 10.5+dy + stretchBy);
        
        Point7    = QPointF(2.1+dx,  9.5+dy);	
        cPoint7_1 = QPointF(2.1+dx,  6.8+dy);
        cPoint7_2 = QPointF(2.2+dx,  2.7+dy);
        Point8    = QPointF(5.8+dx,  1.0+dy);
        
        leftPath.moveTo(Point1);
        leftPath.cubicTo(cPoint1_1, cPoint1_2, Point2);
        leftPath.lineTo(Point3);
        leftPath.cubicTo(cPoint3_1, cPoint3_2, Point4);
        leftPath.lineTo(Point5);
        leftPath.cubicTo(cPoint5_1, cPoint5_2, Point6);
        leftPath.lineTo(Point7);
        leftPath.cubicTo(cPoint7_1, cPoint7_2, Point8);
        leftPath.closeSubpath();
        
        leftLinePath.moveTo(Point1);
        leftLinePath.lineTo(Point2);
        leftLinePath.lineTo(Point3);
        leftLinePath.lineTo(Point4);
        leftLinePath.lineTo(Point5);
        leftLinePath.lineTo(Point6);
        leftLinePath.lineTo(Point7);
        leftLinePath.lineTo(Point8);
        leftLinePath.closeSubpath();
        
        if (m_paintParenthesisFast) m_leftParenthesis->setPath(leftLinePath);
        else m_leftParenthesis->setPath(leftPath);
    } else {
        QPainterPath rightPath, rightLinePath;
        Point1    = QPointF(0.0+dx,  0.0+dy);	//0.0 -> 5.8
        cPoint1_1 = QPointF(3.5+dx,  1.8+dy);//2.3 -> 3.5
        cPoint1_2 = QPointF(5.8+dx,  4.3+dy);
        Point2    = QPointF(5.8+dx,  9.5+dy);
        
        Point3    = QPointF(5.8+dx, 10.5+dy + stretchBy);	
        cPoint3_1 = QPointF(5.8+dx, 15.7+dy + stretchBy);
        cPoint3_2 = QPointF(3.5+dx, 18.2+dy + stretchBy);
        Point4    = QPointF(0.0+dx, 20.0+dy + stretchBy);
        
        Point5    = QPointF(0.0+dx, 19.0+dy + stretchBy);	
        cPoint5_1 = QPointF(3.0+dx, 17.3+dy + stretchBy);//2.8 -> 3.0
        cPoint5_2 = QPointF(3.7+dx, 13.2+dy + stretchBy);//2.1 -> 3.7
        Point6    = QPointF(3.7+dx, 10.5+dy + stretchBy);
        
        Point7    = QPointF(3.7+dx,  9.5+dy);	
        cPoint7_1 = QPointF(3.7+dx,  6.8+dy);
        cPoint7_2 = QPointF(3.6+dx,  2.7+dy);//2.2 -> 3.6
        Point8    = QPointF(0.0+dx,  1.0+dy);
        
        rightPath.moveTo(Point1);
        rightPath.cubicTo(cPoint1_1, cPoint1_2, Point2);
        rightPath.lineTo(Point3);
        rightPath.cubicTo(cPoint3_1, cPoint3_2, Point4);
        rightPath.lineTo(Point5);
        rightPath.cubicTo(cPoint5_1, cPoint5_2, Point6);
        rightPath.lineTo(Point7);
        rightPath.cubicTo(cPoint7_1, cPoint7_2, Point8);
        rightPath.closeSubpath();
        
        rightLinePath.moveTo(Point1);
        rightLinePath.lineTo(Point2);
        rightLinePath.lineTo(Point3);
        rightLinePath.lineTo(Point4);
        rightLinePath.lineTo(Point5);
        rightLinePath.lineTo(Point6);
        rightLinePath.lineTo(Point7);
        rightLinePath.lineTo(Point8);
        rightLinePath.closeSubpath();
        
        if (m_paintParenthesisFast) m_rightParenthesis->setPath(rightLinePath);
        else m_rightParenthesis->setPath(rightPath);
    }
}
void MathLeafEdit::updateBracesPath(const QPointF& topLeft, qreal totalHeight, bool isLeftBrace) {
    //qDebug() << "MathLeafEdit::updateParenthesisPath";
    qreal stretchBy = 0.5 * (totalHeight -20); // Stretch in upper and lower half 0.5 times
    if (stretchBy < 0.0) stretchBy = 0.0;
    qreal dx = topLeft.x();
    qreal dy = topLeft.y();
    QPointF Point1, cPoint1_1, cPoint1_2;
    QPointF Point2, cPoint2_1, cPoint2_2;
    QPointF Point3, cPoint3_1, cPoint3_2;
    QPointF Point4, cPoint4_1, cPoint4_2;
    QPointF Point5, cPoint5_1, cPoint5_2;
    QPointF Point6, cPoint6_1, cPoint6_2;
    QPointF Point7, cPoint7_1, cPoint7_2;
    QPointF Point8, cPoint8_1, cPoint8_2;
    QPointF Point9, cPoint9_1, cPoint9_2;
    QPointF Point10, cPoint10_1, cPoint10_2;
    QPointF Point11, cPoint11_1, cPoint11_2;
    QPointF Point12, cPoint12_1, cPoint12_2;
    QPointF Point13, cPoint13_1, cPoint13_2;
    QPointF Point14, cPoint14_1, cPoint14_2;
    QPointF Point15;
    if (isLeftBrace) {
        QPainterPath leftPath, leftLinePath;
        Point1     = QPointF(5.8+dx,  0.0+dy);	
        cPoint1_1  = QPointF(1.6+dx,  0.0+dy);
        cPoint1_2  = QPointF(0.5+dx,  1.5+dy);
        Point2     = QPointF(0.5+dx,  3.2+dy);
        cPoint2_1  = QPointF(0.5+dx,  4.9+dy);
        cPoint2_2  = QPointF(1.6+dx,  5.7+dy + stretchBy);
        Point3     = QPointF(1.6+dx,  7.3+dy + stretchBy);	
        cPoint3_1  = QPointF(1.6+dx,  9.6+dy + stretchBy);
        cPoint3_2  = QPointF(0.1+dx,  9.3+dy + stretchBy);
        Point4     = QPointF(0.0+dx,  9.3+dy + stretchBy);
        
        Point5     = QPointF(0.0+dx, 10.7+dy + stretchBy);	
        cPoint5_1  = QPointF(0.1+dx, 10.7+dy + stretchBy);
        cPoint5_2  = QPointF(1.6+dx, 10.4+dy + stretchBy);
        Point6     = QPointF(1.6+dx, 12.7+dy + stretchBy);
        cPoint6_1  = QPointF(1.6+dx, 14.3+dy + stretchBy);
        cPoint6_2  = QPointF(0.5+dx, 15.1+dy + 2.0*stretchBy);
        Point7     = QPointF(0.5+dx, 16.8+dy + 2.0*stretchBy);	
        cPoint7_1  = QPointF(0.5+dx, 18.5+dy + 2.0*stretchBy);
        cPoint7_2  = QPointF(1.6+dx, 20.0+dy + 2.0*stretchBy);
        Point8     = QPointF(5.8+dx, 20.0+dy + 2.0*stretchBy);
        
        Point9     = QPointF(5.8+dx, 19.0+dy + 2.0*stretchBy);	
        cPoint9_1  = QPointF(5.6+dx, 18.9+dy + 2.0*stretchBy);
        cPoint9_2  = QPointF(1.8+dx, 18.7+dy + 2.0*stretchBy);
        Point10    = QPointF(1.8+dx, 16.7+dy + 2.0*stretchBy);	
        cPoint10_1 = QPointF(1.8+dx, 15.4+dy + 2.0*stretchBy);
        cPoint10_2 = QPointF(2.7+dx, 14.1+dy + stretchBy);
        Point11    = QPointF(2.7+dx, 12.7+dy + stretchBy);	
        cPoint11_1 = QPointF(2.7+dx, 11.1+dy + stretchBy);
        cPoint11_2 = QPointF(1.7+dx, 10.0+dy + stretchBy);
        Point12    = QPointF(1.0+dx, 10.0+dy + stretchBy);
        cPoint12_1 = QPointF(1.7+dx, 10.0+dy + stretchBy);
        cPoint12_2 = QPointF(2.7+dx,  8.9+dy + stretchBy);
        Point13    = QPointF(2.7+dx,  7.3+dy + stretchBy);	
        cPoint13_1 = QPointF(2.7+dx,  5.9+dy + stretchBy);
        cPoint13_2 = QPointF(1.8+dx,  4.6+dy);
        Point14    = QPointF(1.8+dx,  3.3+dy);
        cPoint14_1 = QPointF(1.8+dx,  1.3+dy);
        cPoint14_2 = QPointF(5.6+dx,  1.1+dy);
        Point15    = QPointF(5.8+dx,  1.0+dy);	
        
        leftPath.moveTo(Point1);
        leftPath.cubicTo(cPoint1_1, cPoint1_2, Point2);
        leftPath.cubicTo(cPoint2_1, cPoint2_2, Point3);
        leftPath.cubicTo(cPoint3_1, cPoint3_2, Point4);
        leftPath.lineTo(Point5);
        leftPath.cubicTo(cPoint5_1, cPoint5_2, Point6);
        leftPath.cubicTo(cPoint6_1, cPoint6_2, Point7);
        leftPath.cubicTo(cPoint7_1, cPoint7_2, Point8);
        leftPath.lineTo(Point9);
        leftPath.cubicTo(cPoint9_1, cPoint9_2, Point10);
        leftPath.cubicTo(cPoint10_1, cPoint10_2, Point11);
        leftPath.cubicTo(cPoint11_1, cPoint11_2, Point12);
        leftPath.cubicTo(cPoint12_1, cPoint12_2, Point13);
        leftPath.cubicTo(cPoint13_1, cPoint13_2, Point14);
        leftPath.cubicTo(cPoint14_1, cPoint14_2, Point15);
        leftPath.closeSubpath();
        
        m_leftBraces->setPath(leftPath);
    } else {
        QPainterPath rightPath, rightLinePath;
        Point1     = QPointF(5.8-5.8+dx,  0.0+dy);	
        cPoint1_1  = QPointF(5.8-1.6+dx,  0.0+dy);
        cPoint1_2  = QPointF(5.8-0.5+dx,  1.5+dy);
        Point2     = QPointF(5.8-0.5+dx,  3.2+dy);
        cPoint2_1  = QPointF(5.8-0.5+dx,  4.9+dy);
        cPoint2_2  = QPointF(5.8-1.6+dx,  5.7+dy + stretchBy);
        Point3     = QPointF(5.8-1.6+dx,  7.3+dy + stretchBy);	
        cPoint3_1  = QPointF(5.8-1.6+dx,  9.6+dy + stretchBy);
        cPoint3_2  = QPointF(5.8-0.1+dx,  9.3+dy + stretchBy);
        Point4     = QPointF(5.8-0.0+dx,  9.3+dy + stretchBy);
        
        Point5     = QPointF(5.8-0.0+dx, 10.7+dy + stretchBy);	
        cPoint5_1  = QPointF(5.8-0.1+dx, 10.7+dy + stretchBy);
        cPoint5_2  = QPointF(5.8-1.6+dx, 10.4+dy + stretchBy);
        Point6     = QPointF(5.8-1.6+dx, 12.7+dy + stretchBy);
        cPoint6_1  = QPointF(5.8-1.6+dx, 14.3+dy + stretchBy);
        cPoint6_2  = QPointF(5.8-0.5+dx, 15.1+dy + 2.0*stretchBy);
        Point7     = QPointF(5.8-0.5+dx, 16.8+dy + 2.0*stretchBy);	
        cPoint7_1  = QPointF(5.8-0.5+dx, 18.5+dy + 2.0*stretchBy);
        cPoint7_2  = QPointF(5.8-1.6+dx, 20.0+dy + 2.0*stretchBy);
        Point8     = QPointF(5.8-5.8+dx, 20.0+dy + 2.0*stretchBy);
        
        Point9     = QPointF(5.8-5.8+dx, 19.0+dy + 2.0*stretchBy);	
        cPoint9_1  = QPointF(5.8-5.6+dx, 18.9+dy + 2.0*stretchBy);
        cPoint9_2  = QPointF(5.8-1.8+dx, 18.7+dy + 2.0*stretchBy);
        Point10    = QPointF(5.8-1.8+dx, 16.7+dy + 2.0*stretchBy);	
        cPoint10_1 = QPointF(5.8-1.8+dx, 15.4+dy + 2.0*stretchBy);
        cPoint10_2 = QPointF(5.8-2.7+dx, 14.1+dy + stretchBy);
        Point11    = QPointF(5.8-2.7+dx, 12.7+dy + stretchBy);	
        cPoint11_1 = QPointF(5.8-2.7+dx, 11.1+dy + stretchBy);
        cPoint11_2 = QPointF(5.8-1.7+dx, 10.0+dy + stretchBy);
        Point12    = QPointF(5.8-1.0+dx, 10.0+dy + stretchBy);
        cPoint12_1 = QPointF(5.8-1.7+dx, 10.0+dy + stretchBy);
        cPoint12_2 = QPointF(5.8-2.7+dx,  8.9+dy + stretchBy);
        Point13    = QPointF(5.8-2.7+dx,  7.3+dy + stretchBy);	
        cPoint13_1 = QPointF(5.8-2.7+dx,  5.9+dy + stretchBy);
        cPoint13_2 = QPointF(5.8-1.8+dx,  4.6+dy);
        Point14    = QPointF(5.8-1.8+dx,  3.3+dy);
        cPoint14_1 = QPointF(5.8-1.8+dx,  1.3+dy);
        cPoint14_2 = QPointF(5.8-5.6+dx,  1.1+dy);
        Point15    = QPointF(5.8-5.8+dx,  1.0+dy);	
        
        rightPath.moveTo(Point1);
        rightPath.cubicTo(cPoint1_1, cPoint1_2, Point2);
        rightPath.cubicTo(cPoint2_1, cPoint2_2, Point3);
        rightPath.cubicTo(cPoint3_1, cPoint3_2, Point4);
        rightPath.lineTo(Point5);
        rightPath.cubicTo(cPoint5_1, cPoint5_2, Point6);
        rightPath.cubicTo(cPoint7_1, cPoint7_2, Point7);
        rightPath.cubicTo(cPoint7_1, cPoint7_2, Point8);
        rightPath.lineTo(Point9);
        rightPath.cubicTo(cPoint9_1, cPoint9_2, Point10);
        rightPath.cubicTo(cPoint10_1, cPoint10_2, Point11);
        rightPath.cubicTo(cPoint11_1, cPoint11_2, Point12);
        rightPath.cubicTo(cPoint12_1, cPoint12_2, Point13);
        rightPath.cubicTo(cPoint13_1, cPoint13_2, Point14);
        rightPath.cubicTo(cPoint14_1, cPoint14_2, Point15);
        rightPath.closeSubpath();
        
        m_rightBraces->setPath(rightPath);
    }
}

void MathLeafEdit::checkChildrenMaxCenterHeight() {
    qreal ch = -1.0E200;
    QFontMetricsF fm(m_font);
    QFont subscriptFont(m_fontName, m_mathFontSize*m_subScriptScale);
    //QFont subscriptFont(m_fontName, fm.height()*m_subScriptScale);
    QFontMetricsF subFm(subscriptFont);
    qreal currentX = m_paddingH;
    qreal maxY = 0.0;
    bool subscriptActive = false;
    
    for (int i=0; i< m_content.size(); i++) {
        if (isStringAtContent(i)) {
            QString text = getStringAtContent(i);
            if(text == "_" && !subscriptActive) { subscriptActive = true; continue; }
            ch = qMax(ch, 0.5*(fm.height() + 3.0*subscriptActive));
        } else if (isMathLeafEditAtContent(i)) {
            subscriptActive = false;
            MathLeafEdit* child = getMathLeafEditAtContent(i);
            ch = qMax(ch, child->centerHeight());
        }
    }
    m_centerHeight = ch + m_paddingV;
    return;
}


QJsonObject MathLeafEdit::toJson() const {
    QJsonObject object;
    object["type"] = static_cast<int>(type());
    object["name"] = m_name;
    object["cursorVisible"] = m_cursorVisible;
    object["isFunction"] = m_isFunction;
    object["isValidFunction"] = m_isValidFunction;
    object["showParenthesis"] = m_showParenthesis;
    object["paintParenthesisFast"] = m_paintParenthesisFast;
    object["fontName"] = m_fontName;
    object["mathFontSize"] = m_mathFontSize;
    object["contStr"] = m_contStr;
    
    QJsonArray contentArray;
    for (int i=0; i<m_content.size(); i++) {
        if (isStringAtContent(i)) {
            contentArray.append(getStringAtContent(i));
        } else if (isMathLeafEditAtContent(i)) {
            MathLeafEdit* child = getMathLeafEditAtContent(i);
            contentArray.append(child->toJson());
        }
    }
    object["content"] = contentArray;
    
    return object;
}

void MathLeafEdit::fromJson(const QJsonObject& object) {
    m_content.clear();
    int type = object["type"].toInt();
    if (type == MathLeafEdit::Type) {
        m_name = object["name"].toString();
        m_cursorVisible = object["cursorVisible"].toBool();
        m_isFunction = object["isFunction"].toBool();
        m_isValidFunction = object["isValidFunction"].toBool();
        m_showParenthesis = object["showParenthesis"].toBool();
        m_paintParenthesisFast = object["paintParenthesisFast"].toBool();
        m_fontName = object["fontName"].toString();
        m_mathFontSize = object["mathFontSize"].toInt();
        QFont font(m_fontName, m_mathFontSize);
        setFont(font);
        m_contStr = object["contStr"].toString();
        
        QJsonArray array = object["content"].toArray();
        for (const QJsonValue &value: array) {
            if (value.isString()) {
                QString s = value.toString();
                insertText(s, true);
            } else if (value.isObject()) {
                QJsonObject childObject = value.toObject();
                MathLeafEdit* child = new MathLeafEdit(m_data, m_parentPageMathItem, this);
                insertMathLeaf(child);
                child->fromJson(childObject);
            }
        }
    } else {
        qDebug() << "MathLeafEdit::fromJson: Error on loading object!";
    }
}

void MathLeafEdit::refreshLayout() {
    if (m_isValidFunction || m_showParenthesis) return;
    bool isLeaf = true;
    for (int i=0; i<m_content.size(); i++) {
        if (isMathLeafEditAtContent(i)) isLeaf = false;
        break;
    }
    if (isLeaf) updateBoundingRectAndLayout();
}

void MathLeafEdit::compute() {
    try {
        m_mathError = m_mathWarning = QString();
        
        // tokenize the current content.
        QString token("");
        QVariantList tokenList;
        for (int i=0; i<m_content.size(); i++) {
            if (isStringAtContent(i)) { token += getStringAtContent(i); }
            if (isMathLeafEditAtContent(i)) {
                if (!token.isEmpty()) { tokenList.append(token); token = ""; }
                tokenList.append(m_content.at(i));
            }
        }
        if (!token.isEmpty()) { tokenList.append(token); token = ""; }
        
        // let's see if this is the rootMathLeaf()
        bool isRoot(this == m_parentPageMathItem->rootMathLeaf());
        if (isRoot) { //qDebug()<< "MathLeafEdit::compute: This is the root MathLeafEditObject.";
            // this is only allowed to be a new variable name, and a MathLeafEdit
            // containing the ":=" function, or an existing variable name and a MathLeafEdit
            // containing the "=" function.
            m_mathError = QString();
            if (tokenList.size()!=2) {
                m_mathError = tr("Root object has to contain exactly 2 tokens:\n"
                "    A variable name followed by a ':=' or '=' function object.");
                m_name = QString();
                m_value.clear();
                return;
            }
            if (isString(tokenList.at(0)) && isMathLeafEdit(tokenList.at(1))) {
                m_name = getString(tokenList.at(0));
                if (canConvertToMathVariable(m_name)) {
                    m_mathError = tr("ERROR: A variable name is expected, but a number is given.");
                    m_name = QString();
                    m_value.clear();
                    return;
                } else {
                    MathLeafEdit* func = getMathLeafEdit(tokenList.at(1));
                    if (func->isAssignmentFunction()) { // ":=" operator
                        if (m_data->contains(m_name)) {
                            m_mathWarning = tr("WARNING: Redefinition of existing variable '%1'.").arg(m_name);
                        }
                        func->compute();
                        m_value = func->getValue();
//                        qDebug() << tr("m_name = %1, m_value = %2").arg(m_name).arg(m_value.at(0));
                        m_data->setValue(m_name, m_value);
                    } else if (func->isResultFunction()) { // "=" operator
                        if (m_data->contains(m_name)) {
                            m_value = m_data->getValue(m_name);
                            func->setValue(m_value);
                            setValue(m_value);
                            qDebug() << "MathLeafEdit::compute '=' operator: m_value =" << m_value;
                            QString resStr = func->resultString(true);
                            qDebug() << "MathLeafEdit::compute '=' operator: resStr =" << resStr;
                            func->getMathLeafEditAtContent(1)->m_content.replace(0, resStr);
                        } else {
                            m_mathError = tr("ERROR: Undefined variable '%1'.").arg(m_name);
                            m_value.clear();
                            QString resStr("UNDEFINED");
                            func->getMathLeafEditAtContent(1)->m_content.replace(0, resStr);
                        }
                    } else {
                        m_mathError = tr("ERROR: Root object contains no assignment (':=') or result ('=').\n"
                        "       A variable name followed by a ':=' or '=' function object is expected.");
                        m_value.clear();
                        m_name = QString();
                    }
                }
            } else {
                m_value.clear();
                return;
            }
        }
        else { // here comes everything that is not in the root MathLeafEdit object.
            if (isExpression()) { // A MathLeafEdit object containing something like -2*a+b+3*c
                bool startsWithOperator = false;
                QList<QString> operatorList;
                QList<MathVariable> operandList;
                
                for (int i=0; i<tokenList.size(); i++) {
                    if (isString(tokenList.at(i))) {
                        QString S = getString(tokenList.at(i));
                        if (canConvertToMathVariable(S)) {
                            if (startsWithOperator && i==1) { // integrate a sign into the first operand
                                if (operatorList.at(0) == "-") S = QString("-") + S;
                                operatorList.removeAt(0); operandList.removeAt(0); startsWithOperator = false;
                            }
                            operandList.append(convertToMathVariable(S));
                        }
                        else {
                            MathVariable value = m_data->getValue(S);
                            if (isValidValue(value)) operandList.append(value);
                            else {
                                m_mathError = tr("ERROR: No valid entry found for '%1'. Not defined yet?").arg(getString(tokenList.at(i)));
                                operandList.append(MathVariable(QList<qreal>{std::numeric_limits<qreal>::max()}));
                            }
                            if (startsWithOperator && i==1) { // integrate a sign into the first operand
                                if (operatorList.at(0) == "-") operandList.replace(1, mul(QList<qreal>{-1.0}, operandList.at(1)));
                                operatorList.removeAt(0); operandList.removeAt(0); startsWithOperator = false;
                            }
                        }
                        operatorList.append(nullptr); // if we found an operand, we place something empty in the operatorList
                    } else if (isMathLeafEdit(tokenList.at(i))) {
                        MathLeafEdit* M = getMathLeafEdit(tokenList.at(i));
                        if (M->isBasicOperator() && i!=tokenList.size()-1) {
                            operatorList.append(M->getStringAtContent(0));
                            operandList.append(MathVariable()); // if we found an operator, an empty item goes into the operandList
                            if (i==0) { // check if we start with a negative sign
                                if (M->getStringAtContent(0) == "-" || M->getStringAtContent(0) == "+") startsWithOperator = true;
                                else m_mathError = tr("ERROR: Mathematical expression cannot start with '%1'.").arg(M->getStringAtContent(0));
                            }
                        } else if (M->isBasicOperator() && i==tokenList.size()-1) {
                            m_mathWarning = tr("WARNING: Mathematical expression cannot end with '%1'.").arg(M->getStringAtContent(0));
                        } else {
                            M->compute();
                            operandList.append(M->getValue()); // found an expression encapsulated in a MathLeafEdit object
                            operatorList.append(nullptr); // if we found an operand, we place something empty in the operatorList
                        }
                    }
                }
                
                // TODO: Check if the operands and operators are always alternating
                // 1) Compute all multiplications
                QList<int> multOpListIDs; // list with all indices where we have a '*' operator
                for(int i=1; i<operatorList.size(); i++) if (operatorList.at(i) == QString(cdotChar) || operatorList.at(i) == " ") multOpListIDs.append(i);
                for(auto it = multOpListIDs.crbegin(); it != multOpListIDs.crend(); ++it) { // make all '*' computations first.
                    int ID = *it;
                    if ((operatorList.at(ID) == QString(cdotChar) || operatorList.at(ID) == " ") && operandList.size() >= 3 && operatorList.size() >= 3) {
                        operandList.replace(ID-1, mul(operandList.at(ID-1), operandList.at(ID+1)));
                        removeComputedOperatorOperands(ID, operatorList, operandList);
                    } else { // something went wrong!
                        operandList.clear();
                        operandList.append(QList<qreal>{std::numeric_limits<qreal>::max()});
                        m_mathError = tr("ERROR: Mathematical expression malformed at a '*' or a ' ' operator.");
                    }
                }
                // 2) Compute all additions and subtractions
                QList<int> addSubOpListIDs; // list with all indices where we have a '+' or '-' operator
                for(int i=1; i<operatorList.size(); i++) if (operatorList.at(i) == "+" || operatorList.at(i) == "-") addSubOpListIDs.append(i);
                for(auto it = addSubOpListIDs.crbegin(); it != addSubOpListIDs.crend(); ++it) { // make all '*' computations first.
                    int ID = *it;
                    if (operatorList.at(ID) == "+" && operandList.size() >= 3 && operatorList.size() >= 3) {
                        operandList.replace(ID-1, add(operandList.at(ID-1), operandList.at(ID+1)));
                        removeComputedOperatorOperands(ID, operatorList, operandList);
                    } else if (operatorList.at(ID) == "-" && operandList.size() >= 3 && operatorList.size() >= 3) {
                        operandList.replace(ID-1, sub(operandList.at(ID-1), operandList.at(ID+1)));
                        removeComputedOperatorOperands(ID, operatorList, operandList);
                    } else { // something went wrong!
                        operandList.clear();
                        operandList.append(QList<qreal>{std::numeric_limits<qreal>::max()});
                        m_mathError = tr("ERROR: Mathematical expression malformed at a '+' or a '-' operator.");
                    }
                }
                // 3) Compute all comparisons (<,>,<=,>=,==)
                QList<int> compareOpListIDs; // list with all indices where we have a comparison operator
                for(int i=1; i<operatorList.size(); i++) {
                    if ( operatorList.at(i) == "<" || operatorList.at(i) == ">" ||
                         operatorList.at(i) == smallerEqualChar ||
                         operatorList.at(i) == greaterEqualChar ||
                         operatorList.at(i) == longEqualChar )
                        compareOpListIDs.append(i);
                }
                // ToDo: Check for things like x < y < 10.3 and replace with x < y & y < 10.3
                if (compareOpListIDs.size()>1) { // 1>a&3<b<10 --> 1>a&3<b&b<10
                    for(int i=compareOpListIDs.size()-2; i>=0; i--) { // {1,5,7}
                        if (compareOpListIDs[i+1] - compareOpListIDs[i] == 2) { // { i=1 }
                            int insertID = compareOpListIDs[i+1]; // insertID = 7 (after "b")
                            operatorList.insert(insertID, nullptr);
                            operatorList.insert(insertID, QString("&"));
                            operandList.insert(insertID, operandList[insertID-1]);
                            operandList.insert(insertID, MathVariable());
                            compareOpListIDs[i+1] = compareOpListIDs[i+1] + 2;
                        }
                    }
                }
                for(auto it = compareOpListIDs.crbegin(); it != compareOpListIDs.crend(); ++it) { // make all comp computations first.
                    int ID = *it;
                    if (operatorList.at(ID) == "<" && operandList.size() >= 3 && operatorList.size() >= 3) {
                        operandList.replace(ID-1, smallerThen(operandList.at(ID-1), operandList.at(ID+1)));
                        removeComputedOperatorOperands(ID, operatorList, operandList);
                    } else if (operatorList.at(ID) == ">" && operandList.size() >= 3 && operatorList.size() >= 3) {
                        operandList.replace(ID-1, greaterThen(operandList.at(ID-1), operandList.at(ID+1)));
                        removeComputedOperatorOperands(ID, operatorList, operandList);
                    } else if (operatorList.at(ID) == smallerEqualChar && operandList.size() >= 3 && operatorList.size() >= 3) {
                        operandList.replace(ID-1, smallerEqual(operandList.at(ID-1), operandList.at(ID+1)));
                        removeComputedOperatorOperands(ID, operatorList, operandList);
                    } else if (operatorList.at(ID) == greaterEqualChar && operandList.size() >= 3 && operatorList.size() >= 3) {
                        operandList.replace(ID-1, greaterEqual(operandList.at(ID-1), operandList.at(ID+1)));
                        removeComputedOperatorOperands(ID, operatorList, operandList);
                    } else if (operatorList.at(ID) == longEqualChar && operandList.size() >= 3 && operatorList.size() >= 3) {
                        operandList.replace(ID-1, equal(operandList.at(ID-1), operandList.at(ID+1)));
                        removeComputedOperatorOperands(ID, operatorList, operandList);
                    } else { // something went wrong!
                        operandList.clear();
                        operandList.append(QList<qreal>{std::numeric_limits<qreal>::max()});
                        m_mathError = tr("ERROR: Mathematical expression malformed at a comparison operator.");
                    }
                }
                // 4) Compute the "&" (AND) operations
                QList<int> boolANDListIDs; // list with all indices where we have a '&' operator
                for(int i=1; i<operatorList.size(); i++) if ( operatorList.at(i) == "&")
                        boolANDListIDs.append(i);
                for(auto it = boolANDListIDs.crbegin(); it != boolANDListIDs.crend(); ++it) { // make all '&' computations first.
                    int ID = *it;
                    if (operatorList.at(ID) == "&" && operandList.size() >= 3 && operatorList.size() >= 3) {
                        operandList.replace(ID-1, boolAnd(operandList.at(ID-1), operandList.at(ID+1)));
                        removeComputedOperatorOperands(ID, operatorList, operandList);
                    } else { // something went wrong!
                        operandList.clear();
                        operandList.append(QList<qreal>{std::numeric_limits<qreal>::max()});
                        m_mathError = tr("ERROR: Mathematical expression malformed at a comparison operator.");
                    }
                }
                // 5) Compute the "|" (OR) operations
                QList<int> boolORListIDs; // list with all indices where we have a '|' operator
                for(int i=1; i<operatorList.size(); i++) if ( operatorList.at(i) == "|")
                    boolORListIDs.append(i);
                for(auto it = boolORListIDs.crbegin(); it != boolORListIDs.crend(); ++it) { // make all '|' computations first.
                    int ID = *it;
                    if (operatorList.at(ID) == "|" && operandList.size() >= 3 && operatorList.size() >= 3) {
                        operandList.replace(ID-1, boolOr(operandList.at(ID-1), operandList.at(ID+1)));
                        removeComputedOperatorOperands(ID, operatorList, operandList);
                    } else { // something went wrong!
                        operandList.clear();
                        operandList.append(QList<qreal>{std::numeric_limits<qreal>::max()});
                        m_mathError = tr("ERROR: Mathematical expression malformed at a comparison operator.");
                    }
                }
                if (operandList.size()>0) {
                    m_value = operandList.at(0);
                } else { // when there are no valid entries
                    m_value = MathVariable({0.0});
                    m_mathError = tr("ERROR: Mathematical expression malformed.");
                }
//                qDebug() << tr("m_value = %1").arg(m_value.at(0));
            } else if (isUnaryFunction()) { // A MathLeafEdit object containing something like log10(x) or sin(a)
                QString func = QString();
                bool argumentsCorrect = false;
                bool funcCorrect = false;
                if (m_content.size() > 0) {
                    if (isStringAtContent(0)) {
                        func = getStringAtContent(0);
                        funcCorrect = isValidFunction(func);
                    }}
                    if (m_content.size() == 2 && isMathLeafEditAtContent(1) && funcCorrect) {
                        getMathLeafEditAtContent(1)->compute();
                        argumentsCorrect = true;
                    } else if (m_content.size() == 3 && isMathLeafEditAtContent(1) && isMathLeafEditAtContent(2) && funcCorrect) {
                        getMathLeafEditAtContent(1)->compute(); getMathLeafEditAtContent(2)->compute();
                        argumentsCorrect = true;
                    } else {
                        m_value.clear();
                        m_value = QList<qreal>{std::numeric_limits<qreal>::max()};
                        m_mathError = tr("ERROR: Mathematical expression (seems to be a unary function) malformed.");
                    }
                    if (funcCorrect && m_content.size() == 3) {
                        MathLeafEdit* arg1 = getMathLeafEditAtContent(1);
                        MathLeafEdit* arg2 = getMathLeafEditAtContent(2);
                        arg1->compute(); arg2->compute();
                        if(func == "log(") {
                            m_value = log(arg2->getValue(), arg1->getValue());
                        } else if (func == "root(") {
                            m_value = root(arg2->getValue(), arg1->getValue());
                        } else if (func == "mod(") {
                            m_value = mod(arg1->getValue(), arg2->getValue());
                        }
                    }
                    if (funcCorrect && m_content.size() == 2) { // any of the other functions
                        MathLeafEdit* arg1 = getMathLeafEditAtContent(1);
                        arg1->compute();
                        if(func == "(") {
                            m_value = arg1->getValue();
                        } else if (func == "sin(") {
//                            qDebug() << tr("sin: m_value = %1").arg(arg1->getValue().at(0));
                            m_value = sin(arg1->getValue());
                        } else if (func == "asin(") {
                            m_value = asin(arg1->getValue());
                        } else if (func == "cos(") {
                            m_value = cos(arg1->getValue());
                        } else if (func == "acos(") {
                            m_value = acos(arg1->getValue());
                        } else if (func == "tan(") {
                            m_value = tan(arg1->getValue());
                        } else if (func == "atan(") {
                            m_value = atan(arg1->getValue());
                        } else if (func == "sinh(") {
                            m_value = sinh(arg1->getValue());
                        } else if (func == "asinh(") {
                            m_value = asinh(arg1->getValue());
                        } else if (func == "cosh(") {
                            m_value = cosh(arg1->getValue());
                        } else if (func == "acosh(") {
                            m_value = acosh(arg1->getValue());
                        } else if (func == "tanh(") {
                            m_value = tanh(arg1->getValue());
                        } else if (func == "atanh(") {
                            m_value = atanh(arg1->getValue());
                        } else if (func == "log10(") {
                            m_value = log10(arg1->getValue());
                        } else if (func == "ln(") {
                            m_value = ln(arg1->getValue());
                        } else if (func == "log2(") {
                            m_value = log2(arg1->getValue());
                        } else if (func == "abs(") {
                            m_value = abs(arg1->getValue());
                        } else if (func == "round(") {
                            m_value = round(arg1->getValue());
                        } else if (func == "floor(") {
                            m_value = floor(arg1->getValue());
                        } else if (func == "ceil(") {
                            m_value = ceil(arg1->getValue());
                        } else if (func == "trunc(") {
                            m_value = trunc(arg1->getValue());
                        } else if (func == "sign(") {
                            m_value = sign(arg1->getValue());
                        } else if (func == "min(") {
                            m_value = min(arg1->getValue());
                        } else if (func == "max(") {
                            m_value = max(arg1->getValue());
                        }
                    }
            } else if (isBinaryFunction()) { // A MathLeafEdit object containing something like a/b or 3^x
                QString funcStr = getStringAtContent(1); 
                MathLeafEdit* arg1 = getMathLeafEditAtContent(0);
                MathLeafEdit* arg2 = getMathLeafEditAtContent(2);
                arg1->compute(); arg2->compute(); 
                if (funcStr == "/") {
                    m_value = div(arg1->getValue(), arg2->getValue());
                } else if (funcStr == "^") {
                    m_value = power(arg1->getValue(), arg2->getValue());
                }
//                qDebug() << tr("base = %1, exponent = %2").arg(arg1->getValue().at(0)).arg(arg2->getValue().at(0));
            } else if (isAssignmentFunction()) {
                MathLeafEdit* parentMathLeafEdit = qobject_cast<MathLeafEdit*>(parentObject());
                m_name = parentMathLeafEdit->getName();
                MathLeafEdit* child = getMathLeafEditAtContent(1);
                child->compute();
                m_value = child->getValue();
            }
        }
    }
    catch (const std::invalid_argument &e) {
        m_value.setErrorValue(std::numeric_limits<qreal>::max());
        qDebug() << "Caught exception:" << e.what();
        m_mathError = QString("ERROR: ") + QString(e.what());
        
    }
    catch (const QException &e) {
        m_value.setErrorValue(std::numeric_limits<qreal>::max());
        qDebug() << "Caught exception:" << e.what();
        
    }
//    qDebug() << "MathLeafEdit::compute (last line): m_value=" << m_value;
    
    // If any error or warning messages occured, let the PageMathItem know about it
    if (!m_mathError.isEmpty()) {
        m_parentPageMathItem->addErrorMessage(m_mathError);
    }
    if (!m_mathWarning.isEmpty()) {
        m_parentPageMathItem->addWarningMessage(m_mathWarning);
    }
}
void MathLeafEdit::removeComputedOperatorOperands(int ID, QList<QString> &operatorList,
                                                  QList<MathVariable> &operandList) {
    operatorList.removeAt(ID+1); operatorList.removeAt(ID);
    operandList.removeAt(ID+1); operandList.removeAt(ID);
}

bool MathLeafEdit::isValidValue(MathVariable L) {
    if (L.size() == 0) return false;
    if (L.size() == 1) {
        if (L.at(0) == std::numeric_limits<qreal>::max()) return false;
    }
    return true;
}
bool MathLeafEdit::isUnaryFunction() {
    if (m_isFunction && m_isValidFunction && m_content.size() == 2) {
        if (isStringAtContent(0) && isMathLeafEditAtContent(1)) {
            if (isValidFunction(getStringAtContent(0))) {
                return true;
            }}} else if (m_isFunction && m_isValidFunction && m_content.size() == 3) {
                if (isStringAtContent(0) && isMathLeafEditAtContent(1) && isMathLeafEditAtContent(2)) {
                    if (getStringAtContent(0) == "log(" || getStringAtContent(0) == "root(" || getStringAtContent(0) == "mod(") {
                        return true;
                    }}}
                    return false;
}
bool MathLeafEdit::isExpression() {
    return !m_isFunction;
}
bool MathLeafEdit::isBasicOperator() { // true if +, - or * is the only entry in m_content
    if(m_isFunction && !m_isValidFunction){
        if (m_content.size() == 1) {
            if (isStringAtContent(0)) {
                QString s(getStringAtContent(0));
                if (s == "+" || s == "-" || s == QString(cdotChar) ||
                    s == " " || s == "<" || s == ">" ||
                    s == longEqualChar || s == smallerEqualChar ||
                    s == greaterEqualChar || s == "&" || s == "|" )
                    return true;
    }}}
    return false;
}
bool MathLeafEdit::isBinaryFunction() {
    if (m_isFunction && !m_isValidFunction && m_content.size() == 3) {
        if (isMathLeafEditAtContent(0) && isStringAtContent(1) && isMathLeafEditAtContent(2)) {
            if (getStringAtContent(1) == "/" || getStringAtContent(1) == "^") {
                return true;
            }}}
            return false;;
}
bool MathLeafEdit::isAssignmentFunction() {
    if (m_isFunction && m_content.size() == 2) {
        if (isStringAtContent(0) && isMathLeafEditAtContent(1)) {
            if (getStringAtContent(0) == ":=") { return true;
            }}}
            return false;
}
bool MathLeafEdit::isResultFunction() {
    if (m_isFunction && m_content.size() == 2) {
        if (isStringAtContent(0) && isMathLeafEditAtContent(1))
            if (getStringAtContent(0) == "=") return true;
    }
    return false;
}

QString MathLeafEdit::resultString(bool units) {
    if (m_content.size()>0) {
        if (isStringAtContent(0)) {
            if (getStringAtContent(0) == "=") {
                // we are in a "display result" MathLeafEdit
                MathLeafEdit* parentMathLeafEdit = qobject_cast<MathLeafEdit*>(parentObject());
                if (parentMathLeafEdit) {
                    if (parentMathLeafEdit->isStringAtContent(0)) {
                        // Calculate units if a resLeaf can be found:
                        QList<qreal> manualUnit({0,0,0,0,0,0,0}); // before a result entry no units exist
                        qreal unitMultiplicator = 1.0;
                        if (m_content.size()>1) {
                            MathLeafEdit *resLeaf; if(isMathLeafEditAtContent(1)) resLeaf = getMathLeafEditAtContent(1);
                            QString tempCont = resLeaf->getStringAtContent(0);
                            resLeaf->m_content.replace(0, "1");
                            resLeaf->compute();
                            MathVariable resLeafValue = resLeaf->getValue();
                            unitMultiplicator = 1.0 / resLeafValue.first();
                            manualUnit = resLeafValue.unit();
                            resLeaf->m_content.replace(0, tempCont);
                        }
                        QString variableName = parentMathLeafEdit->tokenAllAt(0);
                        MathVariable result = m_data->getValue(variableName);
                        QString valStr = QString();
                        if (result.size() == 1) {
                            valStr = tr("%1").arg(result.at(0)*unitMultiplicator);
                        } else if (result.size() > 1 && result.size() <= 4) {
                            valStr = tr("[%1").arg(result.at(0)*unitMultiplicator);
                            for (int i=1; i<result.size(); i++) {
                                valStr += tr(", %1").arg(result.at(i)*unitMultiplicator);
                            }
                            valStr += tr("]");
                        } else if (result.size() > 4) {
                            valStr = tr("[%1, ").arg(result.at(0)*unitMultiplicator);
                            valStr += tr("%1, ").arg(result.at(1)*unitMultiplicator);
                            valStr += tr("%1, ").arg(result.at(2)*unitMultiplicator);
                            valStr += tr("... , %1]").arg(result.last()*unitMultiplicator);
                            //valStr += tr("]");
                            //valStr = tr("[%1, ... ,%2]").arg(result.at(0)*unitMultiplicator, result.last()*unitMultiplicator);
                        }
                        
                        if (units) {
                            // add the unit string (m, kg, s, A, K, mol, cd)
                            QString mul = " "; // QString(cdotChar);
                            QList<qreal> unit = result.unit();
                            if(unit[0]-manualUnit[0] != 0.0) valStr += mul + tr("m^%1").arg(  unit[0]-manualUnit[0]); 
                            if(unit[1]-manualUnit[1] != 0.0) valStr += mul + tr("kg^%1").arg( unit[1]-manualUnit[1]); 
                            if(unit[2]-manualUnit[2] != 0.0) valStr += mul + tr("s^%1").arg(  unit[2]-manualUnit[2]); 
                            if(unit[3]-manualUnit[3] != 0.0) valStr += mul + tr("A^%1").arg(  unit[3]-manualUnit[3]); 
                            if(unit[4]-manualUnit[4] != 0.0) valStr += mul + tr("K^%1").arg(  unit[4]-manualUnit[4]); 
                            if(unit[5]-manualUnit[5] != 0.0) valStr += mul + tr("mol^%1").arg(unit[5]-manualUnit[5]); 
                            if(unit[6]-manualUnit[6] != 0.0) valStr += mul + tr("cd^%1").arg( unit[6]-manualUnit[6]); 
                        }
                        return valStr;
                    }
                }
            }
        }
    }
    return QString();
}
