#include "table.h"
#include <iostream>
#include <climits>
#include <cctype>
#include <sstream>

string Table::getPrimaryKey(const Row& row)
{
    string pk;
    for(int i = 0; i<meta.colCount; i++)
    {
        if(meta.columns[i].isPK)
        {
            pk = row.values[i];
            break;
        }
    }

    return pk;
}

bool Table::validateRow(const Row& row)
{
    if(row.values.size() != meta.colCount)
    {
        cerr<<"ERROR: Invalid number of values! Please insert correct no of values!"<<endl;
        return false;
    }

    for(int i=0;i<meta.colCount;i++)
    {
        if(!validateValue(row.values[i], meta.columns[i]))
        {
            cerr<<"ERROR: Invalid value found!"<<endl;
            return false;
        } 
    }
    return true;
}

bool isBool(const string& s) 
{
    if(s == "true" || s == "false" || s == "0" || s == "1") return true;
    cerr<<"ERROR: Inavlid Boolean expression!"<<endl;
    return false;
}

bool isString(const string& s, int maxSize) 
{
    if(s.length() <= maxSize) return true;
    cerr<<"ERROR: Max string length exceeded!"<<endl;
    return false;
}

// For Checking integer and float we have two options: manual way OR using string stream
// Manual way is little faster and does not allocate any memory but is very lengthy too and there can be some hidden edge cases
//String stream is little slower and allocates some memory on Ram but it is veru concise and handles all edge cases very well

// bool isInt(const string& s) {
//     if (s.empty())
//     {
//         cerr<<"ERROR: Inavalid Integer!"<<endl;
//         return false;
//     } 
    
//     int start = 0;
//     bool isNegative = false;
//     if (s[0] == '-') 
//     { 
//         isNegative = true; 
//         start = 1; 
//     }
//     else if (s[0] == '+') { 
//         start = 1; 
//     }
    
//     if (start >= s.length())
//     {
//         cerr<<"ERROR: Inavalid Integer!"<<endl;
//         return false;
//     } 
    
//     long long ans = 0;
//     for (int i = start; i < s.length(); i++) {
//         if (!isdigit(s[i])) return false;
        
//         // Safety check to prevent long long itself from overflowing
//         if (ans > LLONG_MAX / 10)
//         {
//             cerr<<"ERROR: Inavalid Integer!"<<endl;
//             return false;
//         } 
        
//         ans = ans * 10 + (s[i] - '0');
        
//         // int underflow/overflow checks
//         if ((isNegative && -ans < INT_MIN) || (!isNegative && ans > INT_MAX))
//         {
//             cerr<<"ERROR: Integer out of range!!"<<endl;
//             return false;
//         }
        
//     }
//     return true;
// }

bool isInt(const string& s) 
{
    if (s.empty())
    {
        cerr<<"ERROR: Inavalid Integer!"<<endl;
        return false;
    } 

    stringstream ss(s);
    int d;
    
    if((ss >> d) && ss.eof()) return true;
    
    cerr<<"ERROR: Inavalid Integer!"<<endl;
    return false;
    
}

bool isFloat(const string& s) 
{
    if (s.empty())
    {
        cerr<<"ERROR: Inavalid Float!"<<endl;
        return false;
    } 

    // To properly handle scientific notation (e/E) and float overflows safely,
    // stringstream is the most robust manual-loop equivalent.
    stringstream ss(s);
    float f;
    
    if((ss >> f) && ss.eof()) return true;
    
    cerr<<"ERROR: Inavalid Float!"<<endl;
    return false;
    
}

bool Table::validateValue(const string& value,const ColMeta& col)
{
    switch(col.type){
        case INT:
            return isInt(value);
        case FLOAT:
            return isFloat(value);
        case STRING:
            return isString(value,col.colSize);
        case BOOL:
            return isBool(value);
    }
    return false;
}



