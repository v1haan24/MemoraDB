#include "serializer.h"
#include "table.h"
#include <chrono>

void writeColumn(ostream& file,const ColMeta& col)
{
    file.write(col.name,col_name_sz);
    writeBinary(file,col.type);
    writeBinary(file,col.colSize);
    writeBinary(file,col.offset);
    writeBinary(file,col.isPK);
}

void readColumn(istream& file,ColMeta& col)
{
    file.read(col.name,col_name_sz);
    readBinary(file,col.type);
    readBinary(file,col.colSize);
    readBinary(file,col.offset);
    readBinary(file,col.isPK);
}

uint64_t Table::writeHeader(fstream& file, bool deleted=false)
{
    uint64_t timestamp =
        chrono::duration_cast<chrono::milliseconds>(
            chrono::system_clock::now().time_since_epoch()
        ).count();

    writeBinary(file,timestamp);
    writeBinary(file,deleted);
    return timestamp;
}

void Table::writePayload(fstream& file,const Row& row)
{
    vector<char> stringBuffer; // Reusable buffer to avoid repeated allocating and deallocating memory inside the loop

    for(int i=0;i<meta.colCount;i++)
    {
        if(meta.columns[i].type==INT)
        {
            int x=stoi(row.values[i]);
            writeBinary(file,x);
        }
        else if(meta.columns[i].type==FLOAT)
        {
            float x=stof(row.values[i]);
            writeBinary(file,x);
        }
        else if(meta.columns[i].type==BOOL)
        {
            bool x=(row.values[i]=="true" || row.values[i]=="1");
            writeBinary(file,x);
        }
        else if(meta.columns[i].type==STRING)
        {
            stringBuffer.assign(meta.columns[i].colSize, '\0');
            memcpy(stringBuffer.data(),
            row.values[i].data(),
            min((int)row.values[i].size(),meta.columns[i].colSize-1));

            file.write(stringBuffer.data(),meta.columns[i].colSize);
        }
    }
}

Row Table::readPayload(fstream& file)
{
    Row row;
    for(int i=0;i<meta.colCount;i++){
        if(meta.columns[i].type==INT){
            int x;
            readBinary(file,x);
            row.values.push_back(to_string(x));
        }
        else if(meta.columns[i].type==FLOAT){
            float x;
            readBinary(file,x);
            row.values.push_back(to_string(x));
        }
        else if(meta.columns[i].type==BOOL){
            bool x;
            readBinary(file,x);
            row.values.push_back(x?"true":"false");

        }
        else if(meta.columns[i].type==STRING){
            string temp(meta.columns[i].colSize,'\0');
            file.read(&temp[0], meta.columns[i].colSize);
            temp.resize(strlen(temp.c_str()));
            row.values.push_back(temp);
        }
    }
    return row;
}
