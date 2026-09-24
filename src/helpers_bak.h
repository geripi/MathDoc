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
#include <QRegularExpression>
#include <QList>
#include <QtGlobal>   // for qreal, qMax, etc.
#include <limits>     // for std::numeric_limits
#include <cmath>
#include <algorithm>

// Use the new static map with QChar to QChar mapping
static QMap<QChar, QChar> latinToGreek;

void initializeGreekKeyMap() {
    if (latinToGreek.isEmpty()) {
        latinToGreek = {
            { 'A', 'A'}, { 'B', 'B'}, { 'C', 'C'}, { 'D', QChar(0x0394) },
            { 'E', 'E'}, { 'F', QChar(0x03A6) }, { 'G', QChar(0x0393) }, { 'H', 'H'},
            { 'I', 'I'}, { 'J', QChar(0x03D1) }, { 'K', 'K'}, { 'L', QChar(0x039B) },
            { 'M', 'M'}, { 'N', 'N'}, { 'O', 'O'}, { 'P', QChar(0x03A0) },
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

/**
 * @brief getGreekCharacter Translates a single English alphabet character to its Greek equivalent.
 * @param inputChar A QString containing a single character (e.g., "a", "B", "!").
 * @return A QString containing the Greek equivalent if translatable, otherwise the original input.
 */
QString getGreekCharacter(const QString& inputChar) {
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
bool couldBeNumber(const QString& str) {
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
    return regex.match(str).hasMatch();
}


 // Example Usage (can be placed in main.cpp or a test function)
int testCouldBeNumber() {
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

QList<QString> getValidFunctions(){
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

bool isValidFunction(const QString &str) {
    return getValidFunctions().contains(str);
}

// Global multiplication for QList<qreal>
inline QList<qreal> mul(const QList<qreal>& lhs, const QList<qreal>& rhs)
{
    QList<qreal> result;
    
    // Case 1: both lists same size → element-wise multiply
    if (lhs.size() == rhs.size()) {
        result.reserve(lhs.size());
        for (int i = 0; i < lhs.size(); ++i)
            result.append(lhs[i] * rhs[i]);
    }
    // Case 2: lhs has 1 element → broadcast
    else if (lhs.size() == 1) {
        result.reserve(rhs.size());
        for (int i = 0; i < rhs.size(); ++i)
            result.append(lhs[0] * rhs[i]);
    }
    // Case 3: rhs has 1 element → broadcast
    else if (rhs.size() == 1) {
        result.reserve(lhs.size());
        for (int i = 0; i < lhs.size(); ++i)
            result.append(lhs[i] * rhs[0]);
    }
    // Case 4: incompatible sizes → return QList with max qreal
    else {
        result.append(std::numeric_limits<qreal>::max());
    }
    
    return result;
}

// Global operator+ for QList<qreal>
inline QList<qreal> add(const QList<qreal>& lhs, const QList<qreal>& rhs)
{
    QList<qreal> result;
    
    // Case 1: both lists same size → element-wise multiply
    if (lhs.size() == rhs.size()) {
        result.reserve(lhs.size());
        for (int i = 0; i < lhs.size(); ++i)
            result.append(lhs[i] + rhs[i]);
    }
    // Case 2: lhs has 1 element → broadcast
    else if (lhs.size() == 1) {
        result.reserve(rhs.size());
        for (int i = 0; i < rhs.size(); ++i)
            result.append(lhs[0] + rhs[i]);
    }
    // Case 3: rhs has 1 element → broadcast
    else if (rhs.size() == 1) {
        result.reserve(lhs.size());
        for (int i = 0; i < lhs.size(); ++i)
            result.append(lhs[i] + rhs[0]);
    }
    // Case 4: incompatible sizes → return QList with max qreal
    else {
        result.append(std::numeric_limits<qreal>::max());
    }
    
    return result;
}

// Global operator- for QList<qreal>
inline QList<qreal> sub(const QList<qreal>& lhs, const QList<qreal>& rhs)
{
    QList<qreal> result;
    
    // Case 1: both lists same size → element-wise multiply
    if (lhs.size() == rhs.size()) {
        result.reserve(lhs.size());
        for (int i = 0; i < lhs.size(); ++i)
            result.append(lhs[i] - rhs[i]);
    }
    // Case 2: lhs has 1 element → broadcast
    else if (lhs.size() == 1) {
        result.reserve(rhs.size());
        for (int i = 0; i < rhs.size(); ++i)
            result.append(lhs[0] - rhs[i]);
    }
    // Case 3: rhs has 1 element → broadcast
    else if (rhs.size() == 1) {
        result.reserve(lhs.size());
        for (int i = 0; i < lhs.size(); ++i)
            result.append(lhs[i] - rhs[0]);
    }
    // Case 4: incompatible sizes → return QList with max qreal
    else {
        result.append(std::numeric_limits<qreal>::max());
    }
    
    return result;
}

inline QList<qreal> log(const QList<qreal>& x, const QList<qreal>& a)
{
    QList<qreal> result;
    if (a.size() != 1) {
        // invalid base: return a list with max qreal
        result.append(std::numeric_limits<qreal>::max());
        return result;
    }
    qreal base = a[0];
    // base must be > 0, != 1
    if (base <= 0.0 || base == 1.0) {
        result.append(std::numeric_limits<qreal>::max());
        return result;
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

QList<qreal> div(const QList<qreal> &numerator, const QList<qreal> &denominator) {
    int nSize = numerator.size();
    int dSize = denominator.size();
    // Determine output size
    int outSize = qMax(nSize, dSize);
    // Check for incompatible lengths
    if (nSize != dSize && nSize != 1 && dSize != 1) {
        return { std::numeric_limits<qreal>::max() };
    }
    QList<qreal> result;
    result.reserve(outSize);
    for (int i = 0; i < outSize; ++i) {
        qreal num = numerator[(nSize == 1) ? 0 : i];
        qreal den = denominator[(dSize == 1) ? 0 : i];
        if (den == 0.0) {
            return { std::numeric_limits<qreal>::max() };
        }
        result.append(num / den);
    }
    return result;
}

inline QList<qreal> power(const QList<qreal>& x, const QList<qreal>& a)
{
    QList<qreal> result;
    if (a.size() != 1) {
        // invalid exponent list length
        result.append(std::numeric_limits<qreal>::max());
        return result;
    }
    qreal exp = a[0];
    result.reserve(x.size());
    for (qreal val : x) {
        // std::pow handles many cases (negative bases with integer exponents,
        // fractional exponents for roots, etc.)
        errno = 0;
        qreal p = std::pow(val, exp);
        
        if (errno == EDOM || errno == ERANGE) {
            // domain/range error → return max qreal
            result.append(std::numeric_limits<qreal>::max());
        } else {
            result.append(p);
        }
    }
    return result;
}

inline QList<qreal> root(const QList<qreal>& x, const QList<qreal>& a)
{
    QList<qreal> result;
    if (a.size() != 1) {
        // invalid root list length
        result.append(std::numeric_limits<qreal>::max());
        return result;
    }
    qreal degree = a[0];
    // Root of degree 0 is invalid
    if (degree == 0.0) {
        result.append(std::numeric_limits<qreal>::max());
        return result;
    }
    result.reserve(x.size());
    for (qreal val : x) {
        // For real numbers: only allow val >= 0 or integer degree
        if (val < 0.0 && std::floor(degree) != degree) {
            // negative base with non-integer root is not real
            result.append(std::numeric_limits<qreal>::max());
            continue;
        }
        qreal exponent = 1.0 / degree;
        errno = 0;
        qreal r = std::pow(val, exponent);
        if (errno == EDOM || errno == ERANGE) {
            result.append(std::numeric_limits<qreal>::max());
        } else {
            result.append(r);
        }
    }
    return result;
}

template<typename Func, typename Validator>
QList<qreal> applyFunction(const QList<qreal> &input, Func func, Validator valid) {
    QList<qreal> result;
    result.reserve(input.size());
    
    for (auto val : input) {
        if (!valid(val)) {
            // Invalid input: return QList with a single maximum qreal
            return { std::numeric_limits<qreal>::max() };
        }
        result.append(func(val));
    }
    
    return result;
}

// safe sin (always valid, so validator always returns true)
QList<qreal> sin(const QList<qreal> &x) {
    return applyFunction(x, [](qreal v){ return std::sin(v); }, [](qreal){ return true; });
}
// safe asin (input must be between -1 and 1)
QList<qreal> asin(const QList<qreal> &x) {
    return applyFunction(x, [](qreal v){ return std::asin(v); }, [](qreal v){ return v >= -1.0 && v <= 1.0; });
}
// safe cos (always valid, so validator always returns true)
QList<qreal> cos(const QList<qreal> &x) {
    return applyFunction(x, [](qreal v){ return std::cos(v); }, [](qreal){ return true; });
}
// safe acos (input must be between -1 and 1)
QList<qreal> acos(const QList<qreal> &x) {
    return applyFunction(x, [](qreal v){ return std::acos(v); }, [](qreal v){ return v >= -1.0 && v <= 1.0; });
}
// safe tan (always valid, so validator always returns true)
QList<qreal> tan(const QList<qreal> &x) {
    return applyFunction(x, [](qreal v){ return std::tan(v); }, [](qreal){ return true; });
}
// safe atan (always valid, so validator always returns true)
QList<qreal> atan(const QList<qreal> &x) {
    return applyFunction(x, [](qreal v){ return std::atan(v); }, [](qreal){ return true; });
}

// Hyperbolic functions (always valid)
QList<qreal> sinh(const QList<qreal> &x) {
    return applyFunction(x, [](qreal v){ return std::sinh(v); }, [](qreal){ return true; });
}
QList<qreal> cosh(const QList<qreal> &x) {
    return applyFunction(x, [](qreal v){ return std::cosh(v); }, [](qreal){ return true; });
}
QList<qreal> tanh(const QList<qreal> &x) {
    return applyFunction(x, [](qreal v){ return std::tanh(v); }, [](qreal){ return true; });
}
// Inverse hyperbolic functions (domain validation needed for acosh and atanh)
QList<qreal> asinh(const QList<qreal> &x) {
    return applyFunction(x, [](qreal v){ return std::asinh(v); }, [](qreal){ return true; });
}
QList<qreal> acosh(const QList<qreal> &x) {
    return applyFunction(x, [](qreal v){ return std::acosh(v); }, [](qreal v){ return v >= 1.0; });
}
QList<qreal> atanh(const QList<qreal> &x) {
    return applyFunction(x, [](qreal v){ return std::atanh(v); }, [](qreal v){ return v > -1.0 && v < 1.0; });
}

// Safe natural logarithm
QList<qreal> ln(const QList<qreal> &x) {
    return applyFunction(x, [](qreal v){ return std::log(v); }, [](qreal v){ return v > 0.0; });
}
// Safe base-10 logarithm
QList<qreal> log10(const QList<qreal> &x) {
    return applyFunction(x, [](qreal v){ return std::log10(v); }, [](qreal v){ return v > 0.0; });
}
// Safe base-2 logarithm
QList<qreal> log2(const QList<qreal> &x) {
    return applyFunction(x, [](qreal v){ return std::log2(v); }, [](qreal v){ return v > 0.0; });
}
// Absolute value
QList<qreal> abs(const QList<qreal> &x) {
    return applyFunction(x, [](qreal v){ return std::abs(v); }, [](qreal){ return true; });
}
// Round to nearest integer
QList<qreal> round(const QList<qreal> &x) {
    return applyFunction(x, [](qreal v){ return std::round(v); }, [](qreal){ return true; });
}
// Floor (largest integer ≤ x)
QList<qreal> floor(const QList<qreal> &x) {
    return applyFunction(x, [](qreal v){ return std::floor(v); }, [](qreal){ return true; });
}
// Ceil (smallest integer ≥ x)
QList<qreal> ceil(const QList<qreal> &x) {
    return applyFunction(x, [](qreal v){ return std::ceil(v); }, [](qreal){ return true; });
}
// Truncate (toward zero)
QList<qreal> trunc(const QList<qreal> &x) {
    return applyFunction(x, [](qreal v){ return std::trunc(v); }, [](qreal){ return true; });
}

// Sign function
QList<qreal> sign(const QList<qreal> &x) {
    QList<qreal> result;
    result.reserve(x.size());
    for (auto val : x) {
        if (val > 0.0) result.append(1.0);
        else if (val < 0.0) result.append(-1.0);
        else result.append(0.0);
    }
    return result;
}
// mod function
QList<qreal> mod(const QList<qreal> &numerator, const QList<qreal> &denominator) {
    int nSize = numerator.size();
    int dSize = denominator.size();
    // Determine output size and handle broadcasting
    int outSize = qMax(nSize, dSize);
    
    // Check for incompatible lengths
    if (nSize != dSize && nSize != 1 && dSize != 1) {
        return { std::numeric_limits<qreal>::max() };
    }
    QList<qreal> result;
    result.reserve(outSize);
    
    for (int i = 0; i < outSize; ++i) {
        qreal num = numerator[(nSize == 1) ? 0 : i];
        qreal den = denominator[(dSize == 1) ? 0 : i];
        if (den == 0.0) {
            return { std::numeric_limits<qreal>::max() };
        }
        // Use std::fmod for floating point remainder
        result.append(std::fmod(num, den));
    }
    return result;
}

// Safe min
QList<qreal> min(const QList<qreal> &x) {
    if (x.isEmpty()) {
        return { std::numeric_limits<qreal>::max() };
    }
    qreal minVal = *std::min_element(x.begin(), x.end());
    return { minVal };
}

// Safe max
QList<qreal> max(const QList<qreal> &x) {
    if (x.isEmpty()) {
        return { std::numeric_limits<qreal>::lowest() };
    }
    qreal maxVal = *std::max_element(x.begin(), x.end());
    return { maxVal };
}








#endif // HELPERS_H
