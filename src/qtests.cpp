#include <qcompare.h>
#include "boost/algorithm/string/trim.hpp"
#include "qtests.h"


TestMinimal::TestMinimal(QObject *parent)
{
    // Открываем БД
    testBot.IInitializeDB();

    // read Trello tokens:
    testBot.ReadSettingsFromDB();
}

void TestMinimal::initTestCase() { qDebug("called before everything else"); }

void TestMinimal::cleanupTestCase()
{
    QVERIFY(false);
}

void TestMinimal::testMemSizeVerify()
{
    QVERIFY2(testBot.GetMemSize(testBot.memUsedScriptPath) < 25, "Too much RAM used");  // Пробуем протестить, сколько Мб ОЗУ ест?
}

void TestMinimal::testMemSizeCompare()
{
    /* if (QHostInfo::localHostName() == "ald-c421-173")    */
    /* QCOMPARE(testBot.GetMemSize(testBot.memUsedScriptPath), 10); */
}

void TestMinimal::init()
{ std::cout << "called before EVERY test" << std::endl; }

void TestMinimal::cleanup()
{ std::cout << "called after EVERY test" << std::endl; }
