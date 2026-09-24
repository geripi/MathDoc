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

#include "unit.h"
#include <QRegularExpression>
#include <QStringList>
#include <limits>
#include <cmath>
#include <iostream>

// --- constructors ---
Unit::Unit() : exponents{} {}
Unit::Unit(const std::array<double, COUNT>& exps) : exponents(exps) {}

double Unit::getExp(SIBaseUnit u) const { return exponents[u]; }
void Unit::setExp(SIBaseUnit u, double val) { exponents[u] = val; }

bool Unit::hasError() const { return error_flag; }
void Unit::clearError() { error_flag = false; }

// --- arithmetic ---
Unit Unit::operator*(const Unit& rhs) const {
    std::array<double, COUNT> res{};
    for (size_t i=0;i<COUNT;++i) res[i] = exponents[i] + rhs.exponents[i];
    return Unit(res);
}

Unit Unit::operator/(const Unit& rhs) const {
    std::array<double, COUNT> res{};
    for (size_t i=0;i<COUNT;++i) res[i] = exponents[i] - rhs.exponents[i];
    return Unit(res);
}

Unit Unit::pow(double n) const {
    std::array<double, COUNT> res{};
    for (size_t i=0;i<COUNT;++i) res[i] = exponents[i]*n;
    return Unit(res);
}

Unit Unit::root(double n) const {
    if (n==0.0) throw std::runtime_error("Root degree cannot be zero");
    return pow(1.0/n);
}

Unit Unit::operator+(const Unit& rhs) const {
    Unit result = *this;
    for (size_t i=0;i<COUNT;++i) {
        if (exponents[i]!=rhs.exponents[i]) { result.error_flag=true; break; }
    }
    return result;
}

Unit Unit::operator-(const Unit& rhs) const {
    return (*this)+rhs; // same rules
}

bool Unit::operator==(const Unit& rhs) const {
    for (size_t i=0;i<COUNT;++i) if (exponents[i]!=rhs.exponents[i]) return false;
    return true;
}

bool Unit::operator!=(const Unit& rhs) const { return !(*this==rhs); }

void Unit::print() const {
    static const char* names[COUNT] = {"m","kg","s","A","K","mol","cd"};
    for (size_t i=0;i<COUNT;++i) {
        if (exponents[i]!=0.0) std::cout<<names[i]<<"^"<<exponents[i]<<" ";
    }
    if (error_flag) std::cout<<"(error!)";
    std::cout<<"\n";
}

// --- base units ---
Unit Unit::meter()    { return Unit({1,0,0,0,0,0,0}); }
Unit Unit::kilogram() { return Unit({0,1,0,0,0,0,0}); }
Unit Unit::second()   { return Unit({0,0,1,0,0,0,0}); }
Unit Unit::ampere()   { return Unit({0,0,0,1,0,0,0}); }
Unit Unit::kelvin()   { return Unit({0,0,0,0,1,0,0}); }
Unit Unit::mole()     { return Unit({0,0,0,0,0,1,0}); }
Unit Unit::candela()  { return Unit({0,0,0,0,0,0,1}); }

// --- conversion ---
double Unit::convert(const QString& target) const {
    using Entry = QPair<double,Unit>; // factor, unit
    
    // SI prefixes
    static const QMap<QString,double> prefixes = {
        {"Y", 1e24}, {"Z", 1e21}, {"E", 1e18}, {"P", 1e15},
        {"T", 1e12}, {"G", 1e9}, {"M", 1e6}, {"k", 1e3},
        {"h", 1e2}, {"da",1e1}, {"",1.0},
        {"d",1e-1}, {"c",1e-2}, {"m",1e-3}, {"µ",1e-6},
        {"u",1e-6}, // alt for µ
        {"n",1e-9}, {"p",1e-12}, {"f",1e-15},
        {"a",1e-18}, {"z",1e-21}, {"y",1e-24}
    };
    
    // Base/derived units
    static const QMap<QString,Entry> units = {
        {"m",  {1.0, meter()}},
        {"g",  {1e-3, kilogram()}},   // gram
        {"kg", {1.0, kilogram()}},
        {"t",  {1000.0, kilogram()}},
        {"s",  {1.0, second()}},
        {"min",{60.0, second()}},
        {"h",  {3600.0, second()}},
        {"N",  {1.0, kilogram()*meter()/second().pow(2)}},
        {"W",  {1.0, kilogram()*meter().pow(2)/second().pow(3)}}
    };
    
    // Split by "·"
    QStringList factors = target.split(QChar(0x00B7)); // U+00B7 ·
    double totalFactor = 1.0;
    Unit totalUnit;
    
    for (const QString& part : factors) {
        QString trimmed = part.trimmed();
        if (trimmed.isEmpty()) continue;
        
        // Find longest matching prefix
        QString prefix;
        QString symbol = trimmed;
        for (const QString& p : prefixes.keys()) {
            if (!p.isEmpty() && trimmed.startsWith(p)) {
                QString candidateSymbol = trimmed.mid(p.length());
                if (units.contains(candidateSymbol)) {
                    prefix = p;
                    symbol = candidateSymbol;
                    break;
                }
            }
        }
        
        // If no prefix match, try plain unit
        if (!units.contains(symbol)) {
            return std::numeric_limits<double>::max();
        }
        
        double prefixFactor = prefixes.value(prefix,1.0);
        Entry entry = units[symbol];
        
        totalFactor *= prefixFactor * entry.first;
        totalUnit = totalUnit * entry.second;
    }
    
    // Check compatibility
    if (*this != totalUnit) {
        return std::numeric_limits<double>::max();
    }
    return totalFactor;
}
