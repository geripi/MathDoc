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

#include "matheditcreatenew.h"

#include "mathedit.h"
#include "matheditoperator.h"
#include "matheditconstant.h"
#include "matheditvariable.h"
#include "matheditfunction.h"
#include "matheditfraction.h"
#include "matheditexponent.h"
#include "matheditroot.h"
#include "matheditlog.h"
#include "matheditconditionalfunc.h"

#include <QDebug>

MathEdit* MathEditCreateNew::newMathEdit(QString text, int type, Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent) {
    MathEdit* edit = nullptr;
    
    switch (type) {
        case MathEdit::MEBase:
            edit = new MathEdit(text, d, parentPageMathItem, parent);
            break;
        
        case MathEdit::MEOperator:
            edit = new MathEditOperator(text, d, parentPageMathItem, parent);
            break;
        
        case MathEdit::MEVariable:
            edit = new MathEditVariable(text, d, parentPageMathItem, parent);
            break;
        
        case MathEdit::MEConstant:
            edit = new MathEditConstant(text, d, parentPageMathItem, parent);
            break;
        
        case MathEdit::MEFunction:
            edit = new MathEditFunction(text, d, parentPageMathItem, parent);
            break;
            
        case MathEdit::MEFraction:
            edit = new MathEditFraction(text, d, parentPageMathItem, parent);
            break;
            
        case MathEdit::MEExponent:
            edit = new MathEditExponent(text, d, parentPageMathItem, parent);
            break;
            
        case MathEdit::MERoot:
            edit = new MathEditRoot(text, d, parentPageMathItem, parent);
            break;
            
        case MathEdit::MELog:
            edit = new MathEditLog(text, d, parentPageMathItem, parent);
            break;
            
        case MathEdit::MEConditionalFunc:
            edit = new MathEditConditionalFunc(text, d, parentPageMathItem, parent);
            break;
            
            
        default:
            qWarning() << "MathEditFactory: Unknown MathEdit type: " << MathEdit::typeStr(type);
            return nullptr;
    }
    
    return edit;
}

