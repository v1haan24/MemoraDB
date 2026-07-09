#include "serializer.h"

void writeColumn(ostream& file,const ColMeta& col){
    file.write(col.name,cns);
    writeBinary(file,col.type);
    writeBinary(file,col.size);
    writeBinary(file,col.offset);
    writeBinary(file,col.isPK);
}

void readColumn(istream& file,ColMeta& col){
    file.read(col.name,cns);
    readBinary(file,col.type);
    readBinary(file,col.size);
    readBinary(file,col.offset);
    readBinary(file,col.isPK);
}