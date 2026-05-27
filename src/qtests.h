#ifndef QTESTS_H
#define QTESTS_H

#include <QtTest>
#include "chanserv.h"

class test_minimal : public QObject
{
    Q_OBJECT
public:
    explicit test_minimal(QObject *parent = 0);
    chanserv_bot test_bot;

    void init_test_case();
    void cleanup_test_case();
    void init();
    void cleanup();
private slots:                  // должны быть приватными
    void test_mem_size_verify();
    void test_mem_size_compare();
};

#endif // QTESTS_H
