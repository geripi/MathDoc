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

#include "matheditfactory.h"

#include "mathedit.h"
#include "matheditconstant.h"
#include "matheditoperator.h"
#include "matheditvariable.h"
#include "matheditfunction.h"
#include "matheditfraction.h"
#include "matheditexponent.h"
#include "matheditlog.h"
#include "matheditroot.h"
#include "matheditconditionalfunc.h"

#include <QDebug>
#include <QJsonArray>


MathEdit* MathEditFactory::fromJsonFact(
    const QJsonObject& object,
    Data* data,
    PageMathItem* parentPageMathItem,
    QGraphicsObject* parent)
{
qDebug() << "MathEditFactory::fromJsonFact start";
    // ---------------------------------------------------------
    // 1. Determine the type of the object
    // ---------------------------------------------------------
    
    const int type = object["type"].toInt(-1);
    
    MathEdit* edit = nullptr;
    
    // ---------------------------------------------------------
    // 2. Create the correct concrete MathEdit object
    // ---------------------------------------------------------
    
    switch (type) {
        case MathEdit::MEBase:
            edit = new MathEdit( data, parentPageMathItem, parent);
            break;
        
        case MathEdit::MEConstant:
            edit = new MathEditConstant( data, parentPageMathItem, parent);
            break;
        
        case MathEdit::MEOperator:
            edit = new MathEditOperator( data, parentPageMathItem, parent);
            break;
        
        case MathEdit::MEVariable:
            edit = new MathEditVariable( data, parentPageMathItem, parent);
            break;
        
        case MathEdit::MEFunction:
            edit = new MathEditFunction( data, parentPageMathItem, parent);
            break;
        
        case MathEdit::MEFraction:
            edit = new MathEditFraction( data, parentPageMathItem, parent);
            break;
        
        case MathEdit::MEExponent:
            edit = new MathEditExponent( data, parentPageMathItem, parent);
            break;
        
        case MathEdit::MERoot:
            edit = new MathEditRoot( data, parentPageMathItem, parent);
            break;
        
        case MathEdit::MELog:
            edit = new MathEditLog( data, parentPageMathItem, parent);
            break;
            
        case MathEdit::MEConditionalFunc:
            edit = new MathEditConditionalFunc( data, parentPageMathItem, parent);
            break;
            
        default:
            qWarning() << "MathEditFactory: Unknown MathEdit type:" << type;
            return nullptr;
    }
    
    
    // ---------------------------------------------------------
    // 3. Let the concrete object load its own data
    // ---------------------------------------------------------
    // ---------------------------------------------------------
    // 4. Recursively create all children
    // ---------------------------------------------------------
    createChildTree(object, data, parentPageMathItem, edit);
    
    // ---------------------------------------------------------
    // 5. Return the complete subtree
    // ---------------------------------------------------------
    
    return edit;
}

void MathEditFactory::createChildTree(const QJsonObject& object,
                                      Data* data,
                                      PageMathItem* parentPageMathItem,
                                      MathEdit *edit) {
    
    // 3. Let the concrete object load its own data
    edit->fromJson(object);
qDebug() << "MathEditFactory::fromJsonFact, edit->getContent() = " << edit->getContent();
    
    // 4. Recursively create the unit and all children
    if (object.contains("unit")) {
        MathEdit* unit = fromJsonFact(object["unit"].toObject(), data, parentPageMathItem, edit);
        edit->setUnit(unit);
    }
    
    const QJsonArray childItems = object["childItems"].toArray();
    
    qDebug() << "MathEditFactory::fromJsonFact, edit->type = " << edit->typeStr(edit->type());
    
    for (const QJsonValue& value : childItems) {
        if (edit->typeStr(edit->type()) == "MELog") {
            qDebug() << "MELog";
        }
        if (!value.isObject()) {
            qWarning() << "MathEditFactory: childItems contains a value which is not an object.";
            continue;
        }
        
        MathEdit* child = fromJsonFact(value.toObject(), data, parentPageMathItem, edit);
        
        if (child) {
            edit->addContItem(child);
        }
    }
    
}
