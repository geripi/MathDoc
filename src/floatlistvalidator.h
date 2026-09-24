#pragma once

#include <QValidator>

class FloatListValidator : public QValidator
{
public:
    FloatListValidator(QObject *parent = nullptr)
    : QValidator(parent)
    {
        m_floatValidator.setLocale(QLocale::C);
        m_floatValidator.setNotation(QDoubleValidator::ScientificNotation);
        m_floatValidator.setDecimals(-1);
    }
    
    State validate(QString &input, int &pos) const override
    {
        Q_UNUSED(pos);
        const QStringList values = input.split(',');
        
        for (int i = 0; i < values.size(); ++i) {
            QString value = values[i].trimmed();
            
            // Empty final element is allowed while typing:
            // "1.2,"
            if (value.isEmpty()) {
                if (i == values.size() - 1)
                    return Intermediate;
                
                return Invalid;
            }
            
            int valuePos = 0;
            QString valueCopy = value;
            
            State state = m_floatValidator.validate(valueCopy, valuePos);
            
            if (state == Invalid)
                return Invalid;
            
            if (state == Intermediate)
                return Intermediate;
        }
        
        return Acceptable;
    }
    
private:
    QDoubleValidator m_floatValidator;
};
