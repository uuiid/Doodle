#include "test_convert.h"

#include "src/convert.h"
#include <QtDebug>

test_convert::test_convert()
{

}

void test_convert::test_conver()
{
    dopinyin::convert con = dopinyin::convert();
    QString trs = con.toEn("aa大小d多少");
    qDebug()<<trs;
}
