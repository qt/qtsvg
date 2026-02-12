// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

#include <QList>
#include <QString>
#include <QtTypes>

#ifdef QT_BUILD_INTERNAL
#  include <private/qsvghandler_p.h>
#endif

class tst_QSvgHandler : public QObject
{
Q_OBJECT

public:
    tst_QSvgHandler() = default;
    virtual ~tst_QSvgHandler() = default;

private slots:
#ifdef QT_BUILD_INTERNAL
    void testParseNumbersList_data();
    void testParseNumbersList();
#endif
};

#ifdef QT_BUILD_INTERNAL
void tst_QSvgHandler::testParseNumbersList_data()
{
    QTest::addColumn<QString>("listStr");
    QTest::addColumn<QList<qreal>>("result");
    // "true" if the pointer must advance until the end of the string during parsing
    // "false" if it must not do so or the expected position is unknown, i.e. don't compare
    QTest::addColumn<bool>("pointerAdvancedToEnd");

    // well-formed
    QTest::newRow("float") << ".1" << QList<qreal>{ 0.1 } << true;
    QTest::newRow("integer") << "2" << QList<qreal>{ 2 } << true;
    QTest::newRow("float-space-float") << "1.23 .4" << QList<qreal>{ 1.23, 0.4 } << true;
    QTest::newRow("float-space-integer") << ".34 5" << QList<qreal>{ 0.34, 5 } << true;
    QTest::newRow("integer-space-float") << "6 0.7" << QList<qreal>{ 6, 0.7 } << true;
    QTest::newRow("integer-space-integer") << "8 99" << QList<qreal>{ 8, 99 } << true;
    QTest::newRow("integer-tab-float") << "8\t9.9" << QList<qreal>{ 8, 9.9 } << true;
    QTest::newRow("integer-lf-float") << "7\n8.9" << QList<qreal>{ 7, 8.9 } << true;
    QTest::newRow("integer-cr-float") << "6\r7.9" << QList<qreal>{ 6, 7.9 } << true;
    QTest::newRow("float-comma-float") << ".89,1.23" << QList<qreal>{ 0.89, 1.23 } << true;
    QTest::newRow("float-comma-integer") << "0.89,10" << QList<qreal>{ 0.89, 10 } << true;
    QTest::newRow("integer-comma-float") << "2,.34" << QList<qreal>{ 2, 0.34 } << true;
    QTest::newRow("integer-comma-integer") << "56,78" << QList<qreal>{ 56, 78 } << true;
    QTest::newRow("signed") << "+0.1 -0.2 +3 -4, +0.56, -0.78, +9, -10"
                            << QList<qreal>{ 0.1, -0.2, 3, -4, 0.56, -0.78, 9, -10 } << true;
    QTest::newRow("exponents") << "1e2,3.4e5,-6E7,8e-9" << QList<qreal>{ 100, 3.4e5, -6e7, 8e-9 }
                               << true;
    QTest::newRow("redundant-spaces") << "1   2 , 3\r\n4\t \t5" << QList<qreal>{ 1, 2, 3, 4, 5 }
                                      << true;

    // empty
    QTest::newRow("nullptr") << "nullptr" << QList<qreal>() << false;
    QTest::newRow("nullstring") << QString() << QList<qreal>() << false;
    QTest::newRow("empty") << "" << QList<qreal>() << true;
    QTest::newRow("space") << " " << QList<qreal>() << true;

    QTest::newRow("only-commas") << ",,," << QList<qreal>() << false;
    QTest::newRow("only-spaces") << "   " << QList<qreal>() << true;
    QTest::newRow("only-commas-and-spaces") << " , , " << QList<qreal>() << false;

    // Malformed inputs we used in our tests which were incorrectly treated as valid.
    // After the fix, they should be treated as invalid and their replacements should
    // yield the same results as they did before.
    QTest::newRow("missing-arc-data-fixed")
            << "4 -0 -0 -0 -0 -0 -0" << QList<qreal>{ 4, 0, 0, 0, 0, 0, 0 } << true;
    QTest::newRow("subnormal-radius-fixed")
            << "2 1e-212 .0 .0 .0 .0 .0" << QList<qreal>{ 2, 0, 0, 0, 0, 0, 0 } << true;
    QTest::newRow("from:..+-fixed") << ".0 .0 5 2" << QList<qreal>{ 0, 0, 5, 2 } << true;
    QTest::newRow("by:--,..-fixed") << "-0 -0,.0 .0" << QList<qreal>{ 0, 0, 0, 0 } << true;
    QTest::newRow("animatedSvgContents-fixed")
            << "0 -9.94 -8.06 -18 -18 -18" << QList<qreal>{ 0, -9.94, -8.06, -18, -18, -18 }
            << true;
}

void tst_QSvgHandler::testParseNumbersList()
{
    QFETCH(QString, listStr);
    QFETCH(QList<qreal>, result);
    QFETCH(bool, pointerAdvancedToEnd);

    const bool testNullptr = listStr == "nullptr";
    const QChar *str = testNullptr ? nullptr : listStr.constData();
    QCOMPARE(parseNumbersList(str), result);
    if (testNullptr)
        QCOMPARE(str, nullptr);
    if (pointerAdvancedToEnd)
        QCOMPARE(str, listStr.cend());
}
#endif // QT_BUILD_INTERNAL

QTEST_MAIN(tst_QSvgHandler)
#include "tst_qsvghandler.moc"
