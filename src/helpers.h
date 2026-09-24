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

#ifndef HELPERS_H
#define HELPERS_H

#include <QString>
#include <QChar>
#include <QMap>
#include <QtMath>
#include <QRegularExpression>
#include <QList>
#include <QtGlobal>   // for qreal, qMax, etc.
#include <limits>     // for std::numeric_limits
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include "mathvariable.h"

// Use the new static map with QChar to QChar mapping
inline QMap<QChar, QChar> latinToGreek;

inline void initializeGreekKeyMap() {
    if (latinToGreek.isEmpty()) {
        latinToGreek = {
            { 'A', 'A'}, { 'B', 'B'}, { 'C', 'C'}, { 'D', QChar(0x0394) },
            { 'E', 'E'}, { 'F', QChar(0x03A6) }, { 'G', QChar(0x0393) }, { 'H', 'H'},
            { 'I', 'I'}, { 'J', QChar(0x03D1) }, { 'K', 'K'}, { 'L', QChar(0x039B) },
            { 'M', 'M'}, { 'N', 'N'}, { 'O', QChar(0x03A9)}, { 'P', QChar(0x03A0) },
            { 'Q', QChar(0x03A8) }, { 'R', 'R'}, { 'S', QChar(0x03A3) }, { 'T', 'T'},
            { 'U', QChar(0x0398) }, { 'V', QChar(0x03DD) }, { 'W', QChar(0x03A9) }, { 'X', QChar(0x039E) },
            { 'Y', 'Y'}, { 'Z', 'Z'},
            { 'a', QChar(0x03B1) }, { 'b', QChar(0x03B2) }, { 'c', QChar(0x03C7) }, { 'd', QChar(0x03B4) },
            { 'e', QChar(0x03B5) }, { 'f', QChar(0x03C6) }, { 'g', QChar(0x03B3) }, { 'h', QChar(0x03B7) },
            { 'i', QChar(0x03B9) }, { 'j', QChar(0x03D5) }, { 'k', QChar(0x03BA) }, { 'l', QChar(0x03BB) },
            { 'm', QChar(0x03BC) }, { 'n', QChar(0x03BD) }, { 'o', 'o'}, { 'p', QChar(0x03C0) },
            { 'q', QChar(0x03C8) }, { 'r', QChar(0x03C1) }, { 's', QChar(0x03C3) }, { 't', QChar(0x03C4) },
            { 'u', QChar(0x03B8) }, { 'v', QChar(0x03DB) }, { 'w', QChar(0x03C9) }, { 'x', QChar(0x03BE) },
            { 'y', QChar(0x03C5) }, { 'z', QChar(0x03B6) }
        };
    }
}

inline const QChar cdotChar = QChar(0x00B7);         // "*"
inline const QChar degChar = QChar(0x00B0);          // "°"
inline const QChar smallerEqualChar = QChar(0x2264);   // "<="
inline const QChar greaterEqualChar = QChar(0x2265); // ">="
inline const QChar longEqualChar = QChar(0xFF1D);    // "=="
inline const QChar notEqualChar = QChar(0x2260);    // "!="

inline QString allowedCharacters = 
    QString("ABCDEFGHIJKLMNOPQRSTUVWXYZ_.,:;({)}<>!=^§$%&/\\'\"abcdefghijklmnopqrstuvwxyz0123456789") +
    QString(cdotChar) +
    QString(degChar) +
    QString(smallerEqualChar) +
    QString(greaterEqualChar) +
    QString(longEqualChar) +
    QString(notEqualChar)
    ;

inline QString functionCharacters = QString("abcdefghilnmmorstux012");

/**
 * @brief getGreekCharacter Translates a single English alphabet character to its Greek equivalent.
 * @param inputChar A QString containing a single character (e.g., "a", "B", "!").
 * @return A QString containing the Greek equivalent if translatable, otherwise the original input.
 */
inline QString getGreekCharacter(const QString& inputChar) {
    initializeGreekKeyMap(); // Ensure the map is initialized
    
    if (inputChar.length() != 1) {
        return inputChar; // Not a single character, return as is
    }
    
    QChar qc = inputChar.at(0);
    
    // Look up the corresponding Greek character in the new latinToGreek map
    if (latinToGreek.contains(qc)) {
        return QString(latinToGreek.value(qc));
    }
    
    // If no Greek equivalent is found for the given character, return the original character
    return inputChar;
}

/**
 * @brief couldBeNumber Checks if the entire QString represents a valid beginning
 * of a scientific notation number.
 * This includes integers, floating-point numbers, and partial/complete scientific notation
 * (e.g., "123", "3.14", "-0.5e+3", ".001E-2", "1.234E", ".E").
 * @param str The QString to check.
 * @return True if the string is a valid number or a potential prefix of one, false otherwise.
 */
inline bool couldBeNumber(const QString& str) {
    if (str.isEmpty()) return false;
    // This regular expression matches strings that are:
    // 1. An optional sign (+ or -)
    // 2. Followed by:
    //    a. Zero or more digits, optionally followed by a dot and zero or more digits
    //       (e.g., "123", "123.", "123.45", ".45", ".")
    //    b. Optionally followed by an exponent part:
    //       'e' or 'E', optionally followed by a sign (+ or -), and zero or more digits
    //       (e.g., "E", "E+", "E+1", "1.23E", ".E")
    //
    // The '$' at the end ensures the *entire* string matches the pattern,
    // preventing cases like "34f" or "2.3G" from being considered valid.
    QRegularExpression regex(R"(^[+-]?(?:\d+|\d*\.\d*)(?:[eE](?:[+-]?\d{0,4})?)?$)");
    // Chop the string up at ',' characters:
    QStringList list = str.split(',', Qt::SkipEmptyParts);
    for (QString s: list) {
        if ( !regex.match(s).hasMatch() ) return false; // any substring having no match triggers return false
    }
    return true;
}


 // Example Usage (can be placed in main.cpp or a test function)
inline int testCouldBeNumber() {
    qDebug() << "--- couldBeNumber Tests ---";
    qDebug() << "Test '123abc':" << couldBeNumber("123abc");    // true (digit)
    qDebug() << "Test '.5':" << couldBeNumber(".5");          // true (dot)
    qDebug() << "Test '-10':" << couldBeNumber("-10");        // true (minus)
    qDebug() << "Test '+20':" << couldBeNumber("+20");        // true (plus)
    qDebug() << "Test 'abc':" << couldBeNumber("abc");        // false
    qDebug() << "Test '':" << couldBeNumber("");              // false (empty string)
    qDebug() << "Test ' ': " << couldBeNumber(" ");           // false (space)
    qDebug() << "Test '€10':" << couldBeNumber("€10");        // false (euro symbol)
    
    qDebug() << "\n--- couldBeNumber Tests (now includes partial scientific notation) ---";
    qDebug() << "Test '123':" << couldBeNumber("123");          // true
    qDebug() << "Test '3.14':" << couldBeNumber("3.14");        // true
    qDebug() << "Test '-0.5e+3':" << couldBeNumber("-0.5e+3");  // true
    qDebug() << "Test '.001E-2':" << couldBeNumber(".001E-2");  // true
    qDebug() << "Test '189.53e+2':" << couldBeNumber("189.53e+2"); // true
    qDebug() << "Test '2.3E+33':" << couldBeNumber("2.3E+33");  // true
    qDebug() << "Test '1.234E':" << couldBeNumber("1.234E");    // true (newly handled)
    qDebug() << "Test '123123.324e':" << couldBeNumber("123123.324e"); // true (newly handled)
    qDebug() << "Test '.23e':" << couldBeNumber(".23e");        // true (newly handled)
    qDebug() << "Test '.E':" << couldBeNumber(".E");            // true (newly handled)
    qDebug() << "Test '.e':" << couldBeNumber(".e");            // true (newly handled)
    qDebug() << "Test 'abc':" << couldBeNumber("abc");          // false
    qDebug() << "Test '123abc':" << couldBeNumber("123abc");    // false
    qDebug() << "Test '':" << couldBeNumber("");                // true (empty string is a valid prefix)
    qDebug() << "Test ' ': " << couldBeNumber(" ");             // false
    qDebug() << "Test '34f':" << couldBeNumber("34f");          // false
    qDebug() << "Test '2.3G':" << couldBeNumber("2.3G");        // false
    qDebug() << "Test '.':" << couldBeNumber(".");              // true
    qDebug() << "Test '+.':" << couldBeNumber("+.");            // true
    qDebug() << "Test '-.':" << couldBeNumber("-.");            // true
    return 0;
}

inline bool isOdd(qreal n) {
    // First check: is n an integer?
    if (qFloor(n) == n) {
        // Cast safely to integer
        qint64 intVal = static_cast<qint64>(n);
        return (intVal % 2 != 0);
    }
    return false;
}
inline bool isEven(qreal n) {
    // First check: is n an integer?
    if (qFloor(n) == n) {
        // Cast safely to integer
        qint64 intVal = static_cast<qint64>(n);
        return (intVal % 2 == 0);
    }
    return false;
}

inline QList<QString> getValidFunctions(){
    QList<QString> L( {
                      QString("("),
                      QString("root("),
                      QString("sin("),
                      QString("asin("),
                      QString("cos("),
                      QString("acos("),
                      QString("tan("),
                      QString("atan("),
                      QString("sinh("),
                      QString("asinh("),
                      QString("cosh("),
                      QString("acosh("),
                      QString("tanh("),
                      QString("atanh("),
                      QString("log10("),
                      QString("ln("),
                      QString("log2("),
                      QString("log("),
                      QString("abs("),
                      QString("round("),
                      QString("floor("),
                      QString("ceil("),
                      QString("trunc("),
                      QString("mod("),
                      QString("sign("),
                      QString("min("),
                      QString("max(")
    } );
    return L;
}

inline bool isValidFunction(const QString &str) {
    return getValidFunctions().contains(str);
}

inline bool canConvertToMathVariable(const QString &str) { // 0,3,100 is a list from 0 to 3 with 100 floats
    QStringList list = str.split(',', Qt::SkipEmptyParts);
    if ( !(list.size() == 3 || list.size() == 1) ) return false; // no 3 or 1 entries --> not a MathVariable
    bool canConvertToDouble;
    for (QString s: list) {
        s.toDouble(&canConvertToDouble);
        if ( !canConvertToDouble ) return false;
    }
    return true;
}
inline MathVariable convertToMathVariable(const QString &str) {
    MathVariable res;
    if (canConvertToMathVariable(str)){
        QStringList list = str.split(',', Qt::SkipEmptyParts);
        if (list.size() == 1) {
            res.reserve(1);
            res.append(list[0].toDouble());
        } else if (list.size() == 3) {
            qreal start, end;
            long long n;
            start = list[0].toDouble();
            end = list[1].toDouble();
            n = qRound(list[2].toDouble());
            if (n<2) n = 2;
            qreal delta = (end - start) / static_cast<qreal>(n - 1);
            res.reserve(n);
            for(long long i = 0; i<n; i++) {
                res.append(start + static_cast<qreal>(i)*delta);
            }
        }
    }
    return res;
}

template <typename Func>
inline MathVariable func2(const MathVariable& lhs,
                          const MathVariable& rhs,
                          const MathVariable& boolMask,
                          Func func)
{
    MathVariable result;
    qsizetype resultSize = 0;
    
    const bool sameSize = lhs.size() == rhs.size();
    const bool lhsIsScalar = lhs.size() == 1;
    const bool rhsIsScalar = rhs.size() == 1;
    
    if (sameSize) { resultSize = lhs.size(); }
    else if (lhsIsScalar) { resultSize = rhs.size(); }
    else if (rhsIsScalar) { resultSize = lhs.size(); }
    else {
        throw std::invalid_argument("Incompatible value list sizes in binary operation.");
    }
    if (!boolMask.isEmpty() && boolMask.size() != resultSize && !(lhsIsScalar && rhsIsScalar)) {
        qDebug() << "Incompatible boolMask size in binary operation.";
        throw std::invalid_argument("Incompatible boolMask size in binary operation.");
    }
    result.reserve(resultSize);
    bool maskIsEmpty = boolMask.isEmpty();
    if (lhsIsScalar && rhsIsScalar) maskIsEmpty = true; // ignore boolMask if two scalars are computed
    
    for (qsizetype i = 0; i < resultSize; ++i) {
        qreal res = std::numeric_limits<qreal>::max();
        if (maskIsEmpty || boolMask[i] != 0.0) {
            if (sameSize)         { res = func(lhs[i], rhs[i]); }
            else if (lhsIsScalar) { res = func(lhs[0], rhs[i]); }
            else if (rhsIsScalar) { res = func(lhs[i], rhs[0]); }
        }
        result.append(res);
    }
    return result;
}

template <typename Func>
inline MathVariable func1(const MathVariable& arg,
                          const MathVariable& boolMask,
                          Func func)
{
    MathVariable result;
    const qsizetype resultSize = arg.size();
    
    if (!boolMask.isEmpty() && boolMask.size() != resultSize) {
        throw std::invalid_argument(
            "Incompatible boolMask size in unary operation.");
    }
    result.reserve(resultSize);
    const bool maskIsEmpty = boolMask.isEmpty();
    
    for (qsizetype i = 0; i < resultSize; ++i) {
        qreal res = std::numeric_limits<qreal>::max();
        if (maskIsEmpty || boolMask[i] != 0.0) {
            res = func(arg[i]);
        }
        result.append(res);
    }
    return result;
}

// Global multiplication for MathVariable
inline MathVariable mul(const MathVariable& lhs, const MathVariable& rhs, const MathVariable& boolMask = MathVariable())
{
    MathVariable result = func2(lhs, rhs, boolMask,
                                [](qreal a, qreal b) { return a * b; });
    
    // Always handle the units, even for incompatible sizes
    for(qsizetype i=0; i<7; i++) {
        result.setUnitAt(i,lhs.getUnitAt(i) + rhs.getUnitAt(i));
    }
    return result;
}
inline MathVariable mul(const qreal lhs, const MathVariable& rhs, const MathVariable& boolMask = MathVariable())
{
    return mul(MathVariable(QList<qreal>{lhs}), rhs, boolMask);
}
inline MathVariable mul(const MathVariable& lhs, const qreal rhs, const MathVariable& boolMask = MathVariable())
{
    return mul(lhs, MathVariable(QList<qreal>{rhs}), boolMask);
}

// Global operator+ for QList<qreal>
inline MathVariable add(const MathVariable& lhs, const MathVariable& rhs, const MathVariable& boolMask = MathVariable())
{
    if (!lhs.hasSameUnit(rhs)) {
        throw std::invalid_argument("Units incompatible in '+' operation.");
    }
    MathVariable result = func2(lhs, rhs, boolMask,
                                [](qreal a, qreal b) { return a + b; });
    
    result.setUnit(lhs.unit());
    return result;
}
inline MathVariable add(const qreal lhs, const MathVariable& rhs, const MathVariable& boolMask = MathVariable())
{
    return add(MathVariable(QList<qreal>{lhs}), rhs, boolMask);
}
inline MathVariable add(const MathVariable& lhs, const qreal rhs, const MathVariable& boolMask = MathVariable())
{
    return add(lhs, MathVariable(QList<qreal>{rhs}), boolMask);
}

// Global operator< for QList<qreal>
inline MathVariable smallerThen(const MathVariable& lhs, const MathVariable& rhs, const MathVariable& boolMask = MathVariable())
{
    if ( !lhs.hasSameUnit(rhs) ) {
        throw std::invalid_argument("Units incompatible in '<' operation.");
    }
    
    MathVariable result = func2(lhs, rhs, boolMask,
                                [](qreal a, qreal b) { return (a < b) ? 1.0 : 0.0; });
    
    // Comparison produces a dimensionless result.
    result.setUnit(QList<qreal>({0, 0, 0, 0, 0, 0, 0}));
    return result;
}

// Global operator<= for QList<qreal>
inline MathVariable smallerEqualThen(const MathVariable& lhs, const MathVariable& rhs, const MathVariable& boolMask = MathVariable())
{
    if ( !lhs.hasSameUnit(rhs) ) {
        throw std::invalid_argument("Units incompatible in '<=' operation.");
    }
    
    MathVariable result = func2(lhs, rhs, boolMask,
                                [](qreal a, qreal b) { return (a <= b) ? 1.0 : 0.0; });
    
    // Comparison produces a dimensionless result.
    result.setUnit(QList<qreal>({0, 0, 0, 0, 0, 0, 0}));
    return result;
}

// Global operator> for QList<qreal>
inline MathVariable greaterThen(const MathVariable& lhs, const MathVariable& rhs, const MathVariable& boolMask = MathVariable())
{
    if ( !lhs.hasSameUnit(rhs) ) {
        throw std::invalid_argument("Units incompatible in '>' operation.");
    }
    
    MathVariable result = func2(lhs, rhs, boolMask,
                                [](qreal a, qreal b) { return (a > b) ? 1.0 : 0.0; });
    
    // Comparison produces a dimensionless result.
    result.setUnit(QList<qreal>({0, 0, 0, 0, 0, 0, 0}));
    return result;
}

// Global operator>= for QList<qreal>
inline MathVariable greaterEqualThen(const MathVariable& lhs, const MathVariable& rhs, const MathVariable& boolMask = MathVariable())
{
    if ( !lhs.hasSameUnit(rhs) ) {
        throw std::invalid_argument("Units incompatible in '>=' operation.");
    }
    
    MathVariable result = func2(lhs, rhs, boolMask,
                                [](qreal a, qreal b) { return (a >= b) ? 1.0 : 0.0; });
    
    // Comparison produces a dimensionless result.
    result.setUnit(QList<qreal>({0, 0, 0, 0, 0, 0, 0}));
    return result;
}

// Global operator== for QList<qreal>
inline MathVariable equal(const MathVariable& lhs, const MathVariable& rhs, const MathVariable& boolMask = MathVariable())
{
    if ( !lhs.hasSameUnit(rhs) ) {
        throw std::invalid_argument("Units incompatible in '==' operation.");
    }
    
    MathVariable result = func2(lhs, rhs, boolMask,
                                [](qreal a, qreal b) { return (a == b) ? 1.0 : 0.0; });
    
    // Comparison produces a dimensionless result.
    result.setUnit(QList<qreal>({0, 0, 0, 0, 0, 0, 0}));
    return result;
}

// Global operator!= for QList<qreal>
inline MathVariable notEq(const MathVariable& lhs, const MathVariable& rhs, const MathVariable& boolMask = MathVariable())
{
    if ( !lhs.hasSameUnit(rhs) ) {
        throw std::invalid_argument("Units incompatible in '!=' operation.");
    }
    
    MathVariable result = func2(lhs, rhs, boolMask,
                                [](qreal a, qreal b) { return (a != b) ? 1.0 : 0.0; });
    
    // Comparison produces a dimensionless result.
    result.setUnit(QList<qreal>({0, 0, 0, 0, 0, 0, 0}));
    return result;
}

// Global operator! for QList<qreal>
inline MathVariable boolNot(const MathVariable& rhs, const MathVariable& boolMask = MathVariable())
{
    MathVariable result = func1(rhs, boolMask,
                                [](qreal a) { return (a != 0.0) ? 0.0 : 1.0; });
    
    // Comparison produces a dimensionless result.
    result.setUnit(QList<qreal>({0, 0, 0, 0, 0, 0, 0}));
    return result;
}

// Global operator== for QList<qreal>
inline MathVariable boolAnd(const MathVariable& lhs, const MathVariable& rhs, const MathVariable& boolMask = MathVariable())
{
    if ( !lhs.hasSameUnit(rhs) ) {
        throw std::invalid_argument("Units incompatible in 'AND' operation.");
    }
    
    MathVariable result = func2(lhs, rhs, boolMask,
                                [](qreal a, qreal b) { return (a != 0.0 && b != 0.0) ? 1.0 : 0.0; });
    
    // Comparison produces a dimensionless result.
    result.setUnit(QList<qreal>({0, 0, 0, 0, 0, 0, 0}));
    return result;
}

// Global operator== for QList<qreal>
inline MathVariable boolOr(const MathVariable& lhs, const MathVariable& rhs, const MathVariable& boolMask = MathVariable())
{
    if ( !lhs.hasSameUnit(rhs) ) {
        throw std::invalid_argument("Units incompatible in 'OR' operation.");
    }
    
    MathVariable result = func2(lhs, rhs, boolMask,
                                [](qreal a, qreal b) { return (a != 0.0 || b != 0.0) ? 1.0 : 0.0; });
    
    // Comparison produces a dimensionless result.
    result.setUnit(QList<qreal>({0, 0, 0, 0, 0, 0, 0}));
    return result;
}

// Global operator- for MathVariable
inline MathVariable sub(const MathVariable& lhs, const MathVariable& rhs, const MathVariable& boolMask = MathVariable())
{
    if (!lhs.hasSameUnit(rhs)) {
        throw std::invalid_argument("Units incompatible in '-' operation.");
    }
    MathVariable result = func2(lhs, rhs, boolMask,
                                [](qreal a, qreal b) { return a - b; });
    
    result.setUnit(lhs.unit());
    return result;
}
inline MathVariable sub(const qreal lhs, const MathVariable& rhs, const MathVariable& boolMask = MathVariable())
{
    return sub(MathVariable(QList<qreal>{lhs}), rhs, boolMask);
}
inline MathVariable sub(const MathVariable& lhs, const qreal rhs, const MathVariable& boolMask = MathVariable())
{
    return sub(lhs, MathVariable(QList<qreal>{rhs}), boolMask);
}

inline MathVariable log(const MathVariable& x, const MathVariable& a, const MathVariable& boolMask = MathVariable())
{
    Q_UNUSED(boolMask);
    
    MathVariable result;
    if (a.size() != 1) { // invalid base: return a list with max qreal
        throw std::invalid_argument("Invalid base in 'log' operation: Base is a list of values or empty.");
    }
    if (!x.isDimensionless()) { // Argument not dimensionless
        throw std::invalid_argument("Argument not dimensionless in 'log' operation.");
    }
    if (!a.isDimensionless()) { // Base not dimensionless
        throw std::invalid_argument("Base not dimensionless in 'log' operation.");
    }
    qreal base = a[0];
    // base must be > 0, != 1
    if (base <= 0.0 || base == 1.0) {
        throw std::invalid_argument("Invalid base in 'log' operation: base <= 0.0 or base == 1.0.");
    }
    
    qreal logBaseInv = 1.0/std::log(base);
    result.reserve(x.size());
    for (qreal val : x) {
        if (val > 0.0) {
            result.append(std::log(val) * logBaseInv);
        } else {
            // log undefined → store max qreal
            result.append(std::numeric_limits<qreal>::max());
        }
    }
    return result;
}

inline MathVariable div(const MathVariable &numerator, const MathVariable &denominator, const MathVariable& boolMask = MathVariable())
{
    MathVariable result = func2(numerator, denominator, boolMask,
                                [](qreal a, qreal b) {
                                    if (b == 0.0) { return std::numeric_limits<qreal>::max(); }
                                    else          { return a / b; }
                                });
    
    for (int i = 0; i < 7; ++i) {
        result.setUnitAt(i, numerator.getUnitAt(i) - denominator.getUnitAt(i));
    }
    return result;
}
inline MathVariable div(const qreal lhs, const MathVariable& rhs, const MathVariable& boolMask = MathVariable())
{
    return div(MathVariable(QList<qreal>{lhs}), rhs, boolMask);
}
inline MathVariable div(const MathVariable& lhs, const qreal rhs, const MathVariable& boolMask = MathVariable())
{
    return div(lhs, MathVariable(QList<qreal>{rhs}), boolMask);
}

inline MathVariable power(const MathVariable& x, const MathVariable& a, const MathVariable& boolMask = MathVariable())
{
    if (!a.isDimensionless()) { // Exponent not dimensionless
        throw std::invalid_argument("Exponent not dimensionless in 'power' operation.");
    }
    if (!x.isDimensionless() && (a.size()!=1)) { // Base has dimension and exponent is a list --> leads to incompatible dimensions
        throw std::invalid_argument("Base list has dimension and exponent is a list in 'power' operation--> leads to incompatible dimensions.");
    }
    const bool exponentIsScalar = a.size() == 1;
    
    MathVariable result = func2(x, a, boolMask,
                                [](qreal val, qreal exp) {
                                errno = 0;
                                qreal p = std::pow(val, exp);
                                if (errno == EDOM || errno == ERANGE) { return std::numeric_limits<qreal>::max(); }
                                else { return p; }
                            });
    
    // If the exponent is scalar, the unit can be calculated.
    if (exponentIsScalar) {
        const qreal exponent = a[0];
        for (int i = 0; i < 7; ++i) {
            result.setUnitAt(i,
                             x.getUnitAt(i) * exponent);
        }
    }
    else {// x must be dimensionless here.
        result.setUnit(QList<qreal>({0, 0, 0, 0, 0, 0, 0}));
    }
    return result;
}

inline MathVariable root(const MathVariable& x, const MathVariable& a, const MathVariable& boolMask = MathVariable())
{
    MathVariable one(QList<qreal>{1.0});
    MathVariable exponent = div(one, a);
    
    MathVariable result = power(x, exponent, boolMask);
    return result;
}

// safe sin (always valid, so validator always returns true)
inline MathVariable sin(const MathVariable &x, const MathVariable& boolMask = MathVariable())
{
    if (!x.isDimensionless()) { // Argument not dimensionless
        throw std::invalid_argument("Argument not dimensionless in 'sin' operation.");
    }
    MathVariable result = func1(x, boolMask, [](qreal val) { return std::sin(val); });
    return result;
}
// safe asin (input must be between -1 and 1)
inline MathVariable asin(const MathVariable &x, const MathVariable& boolMask = MathVariable())
{
    if (!x.isDimensionless()) { // Argument not dimensionless
        throw std::invalid_argument("Argument not dimensionless in 'asin' operation.");
    }
    MathVariable result = func1(x, boolMask, [](qreal val) {
                                    if (val < -1.0 || val > 1.0) {
                                        return std::numeric_limits<qreal>::max();
                                    }
                                    return std::asin(val);
                                });
    return result;
}
// safe cos (always valid, so validator always returns true)
inline MathVariable cos(const MathVariable &x, const MathVariable& boolMask = MathVariable()) {
    if (!x.isDimensionless()) { // Argument not dimensionless
        throw std::invalid_argument("Argument not dimensionless in 'cos' operation.");
    }
    MathVariable result = func1(x, boolMask, [](qreal val) { return std::cos(val); });
    return result;
}
// safe acos (input must be between -1 and 1)
inline MathVariable acos(const MathVariable &x, const MathVariable& boolMask = MathVariable()) {
    if (!x.isDimensionless()) { // Argument not dimensionless
        throw std::invalid_argument("Argument not dimensionless in 'acos' operation.");
    }
    MathVariable result = func1(x, boolMask, [](qreal val) {
        if (val < -1.0 || val > 1.0) {
            return std::numeric_limits<qreal>::max();
        }
        return std::acos(val);
    });
    return result;
}
// safe tan (always valid, so validator always returns true)
inline MathVariable tan(const MathVariable &x, const MathVariable& boolMask = MathVariable()) {
    if (!x.isDimensionless()) { // Argument not dimensionless
        throw std::invalid_argument("Argument not dimensionless in 'tan' operation.");
    }
    MathVariable result = func1(x, boolMask, [](qreal val) { return std::tan(val); });
    return result;
}
// safe atan (always valid, so validator always returns true)
inline MathVariable atan(const MathVariable &x, const MathVariable& boolMask = MathVariable()) {
    if (!x.isDimensionless()) { // Argument not dimensionless
        throw std::invalid_argument("Argument not dimensionless in 'atan' operation.");
    }
    MathVariable result = func1(x, boolMask, [](qreal val) { return std::atan(val); });
    return result;
}

// Hyperbolic functions (always valid)
inline MathVariable sinh(const MathVariable &x, const MathVariable& boolMask = MathVariable()) {
    if (!x.isDimensionless()) { // Argument not dimensionless
        throw std::invalid_argument("Argument not dimensionless in 'sinh' operation.");
    }
    MathVariable result = func1(x, boolMask, [](qreal val) { return std::sinh(val); });
    return result;
}
inline MathVariable cosh(const MathVariable &x, const MathVariable& boolMask = MathVariable()) {
    if (!x.isDimensionless()) { // Argument not dimensionless
        throw std::invalid_argument("Argument not dimensionless in 'cosh' operation.");
    }
    MathVariable result = func1(x, boolMask, [](qreal val) { return std::cosh(val); });
    return result;
}
inline MathVariable tanh(const MathVariable &x, const MathVariable& boolMask = MathVariable()) {
    if (!x.isDimensionless()) { // Argument not dimensionless
        throw std::invalid_argument("Argument not dimensionless in 'tanh' operation.");
    }
    MathVariable result = func1(x, boolMask, [](qreal val) { return std::tanh(val); });
    return result;
}
// Inverse hyperbolic functions (domain validation needed for acosh and atanh)
inline MathVariable asinh(const MathVariable &x, const MathVariable& boolMask = MathVariable()) {
    if (!x.isDimensionless()) { // Argument not dimensionless
        throw std::invalid_argument("Argument not dimensionless in 'asinh' operation.");
    }
    MathVariable result = func1(x, boolMask, [](qreal val) { return std::asinh(val); });
    return result;
}
inline MathVariable acosh(const MathVariable &x, const MathVariable& boolMask = MathVariable()) {
    if (!x.isDimensionless()) { // Argument not dimensionless
        throw std::invalid_argument("Argument not dimensionless in 'acosh' operation.");
    }
    MathVariable result = func1(x, boolMask, [](qreal val) {
        if (val < 1.0) {
            return std::numeric_limits<qreal>::max();
        }
        return std::acosh(val);
    });
    return result;
}
inline MathVariable atanh(const MathVariable &x, const MathVariable& boolMask = MathVariable()) {
    if (!x.isDimensionless()) { // Argument not dimensionless
        throw std::invalid_argument("Argument not dimensionless in 'atanh' operation.");
    }
    MathVariable result = func1(x, boolMask, [](qreal val) {
        if (val <= -1.0 || val >= 1.0) {
            return std::numeric_limits<qreal>::max();
        }
        return std::atanh(val);
    });
    return result;
}

// Safe natural logarithm
inline MathVariable ln(const MathVariable &x, const MathVariable& boolMask = MathVariable()) {
    if (!x.isDimensionless()) { // Argument not dimensionless
        throw std::invalid_argument("Argument not dimensionless in 'ln' operation.");
    }
    MathVariable result = func1(x, boolMask, [](qreal val) {
        if (val <= 0.0) {
            return std::numeric_limits<qreal>::max();
        }
        return std::log(val);
    });
    return result;
}
// Safe base-10 logarithm
inline MathVariable log10(const MathVariable &x, const MathVariable& boolMask = MathVariable()) {
    if (!x.isDimensionless()) { // Argument not dimensionless
        throw std::invalid_argument("Argument not dimensionless in 'log10' operation.");
    }
    MathVariable result = func1(x, boolMask, [](qreal val) {
        if (val <= 0.0) {
            return std::numeric_limits<qreal>::max();
        }
        return std::log10(val);
    });
    return result;
}
// Safe base-2 logarithm
inline MathVariable log2(const MathVariable &x, const MathVariable& boolMask = MathVariable()) {
    if (!x.isDimensionless()) { // Argument not dimensionless
        throw std::invalid_argument("Argument not dimensionless in 'log2' operation.");
    }
    MathVariable result = func1(x, boolMask, [](qreal val) {
        if (val <= 0.0) {
            return std::numeric_limits<qreal>::max();
        }
        return std::log2(val);
    });
    return result;
}
// Absolute value
inline MathVariable abs(const MathVariable &x, const MathVariable& boolMask = MathVariable()) {
    MathVariable result = func1(x, boolMask, [](qreal val) { return std::abs(val); });
    result.setUnit(x.unit());
    return result;
}
// Round to nearest integer
inline MathVariable round(const MathVariable &x, const MathVariable& boolMask = MathVariable()) {
    MathVariable result = func1(x, boolMask, [](qreal val) { return std::round(val); });
    result.setUnit(x.unit());
    return result;
}
// Floor (largest integer ≤ x)
inline MathVariable floor(const MathVariable &x, const MathVariable& boolMask = MathVariable()) {
    MathVariable result = func1(x, boolMask, [](qreal val) { return std::floor(val); });
    result.setUnit(x.unit());
    return result;
}
// Ceil (smallest integer ≥ x)
inline MathVariable ceil(const MathVariable &x, const MathVariable& boolMask = MathVariable()) {
    MathVariable result = func1(x, boolMask, [](qreal val) { return std::ceil(val); });
    result.setUnit(x.unit());
    return result;
}
// Truncate (toward zero)
inline MathVariable trunc(const MathVariable &x, const MathVariable& boolMask = MathVariable()) {
    MathVariable result = func1(x, boolMask, [](qreal val) { return std::trunc(val); });
    result.setUnit(x.unit());
    return result;
}

// Sign function
inline MathVariable sign(const MathVariable &x, const MathVariable& boolMask = MathVariable()) {
    MathVariable result = func1(x, boolMask, [](qreal val) {
                                    if (val > 0.0) return 1.0;
                                    if (val < 0.0) return -1.0;
                                    return 0.0;
                                });
    result.setUnit(QList<qreal>({0, 0, 0, 0, 0, 0, 0}));
    return result;
}
// mod function
inline MathVariable mod(const MathVariable &numerator, const MathVariable &denominator, const MathVariable& boolMask = MathVariable()) {
    if (!numerator.hasSameUnit(denominator)) {
        throw std::invalid_argument("Units incompatible in 'mod' operation.");
    }
    MathVariable result = func2(numerator, denominator, boolMask,
                                [](qreal a, qreal b) {
                                    if (b == 0.0) { return std::numeric_limits<qreal>::max(); }
                                    else { return std::fmod(a, b); }
                                });
    result.setUnit(numerator.unit());
    return result;
}

// Safe min
inline MathVariable min(const MathVariable &x, const MathVariable& boolMask = MathVariable()) {
    if (x.isEmpty()) {
        throw std::invalid_argument("Empty variable in 'min' operation.");
    }
    if (!boolMask.isEmpty() && boolMask.size() != x.size()) {
        throw std::invalid_argument("Incompatible boolMask size in 'min' operation.");
    }
    qreal minValue = std::numeric_limits<qreal>::max();
    bool found = false;
    
    for (qsizetype i = 0; i < x.size(); ++i) {
        if (!boolMask.isEmpty() && boolMask[i] == 0.0) { continue; }
        if (x[i] < minValue) {
            minValue = x[i];
            found = true;
        }
    }
    
    if (!found) { throw std::invalid_argument("No values selected by boolMask in 'min' operation."); }
    
    MathVariable result;
    result.append(minValue);
    result.setUnit(x.unit());
    return result;
}

// Safe max
inline MathVariable max(const MathVariable &x, const MathVariable& boolMask = MathVariable()) {
    if (x.isEmpty()) {
        throw std::invalid_argument("Empty variable in 'max' operation.");
    }
    if (!boolMask.isEmpty() && boolMask.size() != x.size()) {
        throw std::invalid_argument("Incompatible boolMask size in 'max' operation.");
    }
    qreal maxValue = -std::numeric_limits<qreal>::max();
    bool found = false;
    
    for (qsizetype i = 0; i < x.size(); ++i) {
        if (!boolMask.isEmpty() && boolMask[i] == 0.0) { continue; }
        if (x[i] > maxValue) {
            maxValue = x[i];
            found = true;
        }
    }
    
    if (!found) { throw std::invalid_argument("No values selected by boolMask in 'max' operation."); }
    
    MathVariable result;
    result.append(maxValue);
    result.setUnit(x.unit());
    return result;
}


#endif // HELPERS_H
