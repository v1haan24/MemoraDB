#include "serializer.h"

using namespace std;

void writeColumnInfo(ofstream& file, const ColumnInfo& col) {
    file.write(col.colName, cns);
    writeBinary(file, col.dType);
    writeBinary(file, col.sizeBytes);
    writeBinary(file, col.offsetValue);
    writeBinary(file, col.isPrimaryKey);
}

void readColumnInfo(ifstream& file, ColumnInfo& col) {
    file.read(col.colName, cns);
    readBinary(file, col.dType);
    readBinary(file, col.sizeBytes);
    readBinary(file, col.offsetValue);
    readBinary(file, col.isPrimaryKey);
}