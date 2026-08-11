// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>

#include <QColor>
#include <QList>
#include <QString>
#include <QXmlStreamAttributes>
#include <QtTypes>

#ifdef QT_BUILD_INTERNAL
#  include <QtSvg/private/qsvganimate_p.h>
#  include <QtSvg/private/qsvgdocument_p.h>
#  include <QtSvg/private/qsvghandler_p.h>
#  include <QtSvg/private/qsvgutils_p.h>
#endif

#include <cmath>
#include <limits>
#include <memory>
#include <utility>

class tst_QSvgHandler : public QObject
{
Q_OBJECT

public:
    tst_QSvgHandler() = default;
    virtual ~tst_QSvgHandler() = default;

private slots:
#ifdef QT_BUILD_INTERNAL
    void testToDouble_data();
    void testToDouble();
    void testCreateAnimateTransformNode_data();
    void testCreateAnimateTransformNode();
    void testParseNumbersList_data();
    void testParseNumbersList();
    void testResolveColor_data();
    void testResolveColor();
#endif
};

#ifdef QT_BUILD_INTERNAL
void tst_QSvgHandler::testToDouble_data()
{
    // make sure we can use NAN (no fast-math)
    static_assert(NAN != NAN);
    static_assert(qreal(NAN) != qreal(NAN));

    QTest::addColumn<QString>("numString");
    QTest::addColumn<qreal>("value");

    using S = std::pair<QString, qreal>;

    const QList<S> signs = { { "", 1. }, { "+", 1. }, { "-", -1. }, { "$", NAN } };
    const QList<S> digits = {
        { "0", 0. },       { "00", 0. },    { "1", 1. },     { "01", 1. },
        { "23", 23. },     { "0023", 23. }, { "456", 456. }, { "7890", 7890. },
        { "9999", 9999. }, { "4x4", NAN },  { "acdc", NAN }, { "ACDC", NAN }
    };

    QList<S> integer = {
        { "-32768", -32768. },
        { "32767", 32767. },
        { "+32767", 32767. },
    };
    integer.reserve(integer.size() + signs.size() * digits.size());
    for (const auto &sign : signs) {
        for (const auto &digitPart : digits) {
            // TODO: Test empty string which is not a valid integer
            integer.push_back({ sign.first + digitPart.first,
                                sign.second * digitPart.second });
        }
    }

    QList<S> decimal_number = integer;
    // limits of conforming SVG Tiny 1.2 content
    decimal_number.push_back({ "-32767.9999", -32767.9999 });
    decimal_number.push_back({ "32767.9999", 32767.9999 });
    decimal_number.push_back({ "+32767.9999", 32767.9999 });
    decimal_number.reserve(decimal_number.size()
                           + signs.size() * digits.size() * (digits.size() + 1));
    // TODO: Test decimal point without following digit which
    //       is not a valid decimal number: QTBUG-143993
    for (const auto &sign : signs) {
        for (const auto &fractDigits : digits) {
            const qreal fractPart =
                    fractDigits.second * std::pow(10., -fractDigits.first.toLatin1().length());
            decimal_number.push_back({ sign.first + "." + fractDigits.first,
                                       sign.second * fractPart });
            for (const auto &wholeDigits : digits) {
                decimal_number.push_back({
                    sign.first + wholeDigits.first + "." + fractDigits.first,
                    sign.second * (wholeDigits.second + fractPart)
                });
            }
        }
    }

    // TODO: Test 'E' or 'e' without following digit which is not a valid scientific number
    constexpr qsizetype exponentCount = 2;
    constexpr QChar exponentChars[exponentCount]{ 'E', 'e' };
    // current implementation's limits
    QList<S> scientific_number = {
        { "-3.4028235e38", -3.4028235e38 },
        { "3.4028235e38", 3.4028235e38 },
        { "+3.4028235e38", 3.4028235e38 },
    };
    scientific_number.reserve(scientific_number.size()
                              + integer.size() * decimal_number.size() * exponentCount);
    for (const auto &exponent : std::as_const(integer)) {
        // The current implementation handles values up to about +/-3.4e(+/-38) mathematically
        // correct, see above. The following check avoids values exceeding these limits because they
        // were known to fail anyway.
        // I don't completely rely on the numeric_limits tested below because the current
        // implementation returns zero for strings like "0e9999" which is correct math but might
        // not be valid svg.
        if ((exponent.second < -38 || 38 < exponent.second))
            continue;
        for (const auto &mantissa : std::as_const(decimal_number)) {
            const qreal value = mantissa.second * std::pow(10., exponent.second);
            if (value < std::numeric_limits<float>::lowest()
                || std::numeric_limits<float>::max() < value)
                continue;
            for (auto e : exponentChars)
                scientific_number.push_back({ mantissa.first + e + exponent.first,
                                              value });
        }
    }
    const auto number{ decimal_number + scientific_number };
    for (const auto &n : number)
        QTest::newRow(n.first.toStdString().c_str()) << n.first << n.second;
}

void tst_QSvgHandler::testToDouble()
{
    QFETCH(QString, numString);
    QFETCH(qreal, value);

    bool ok = false;
    const qreal actual = QSvgUtils::toDouble(numString, &ok);
    QCOMPARE(ok, !std::isnan(value));
    if (ok)
        QCOMPARE(actual, value);
}

void tst_QSvgHandler::testCreateAnimateTransformNode_data()
{
    // The following inputs do not test all of createAnimateTransformNode()
    // - These only are cases which cause the code to pass a pointer by reference into
    //   parseNumberTriplet() and then use the pointer which was changed by that function. This
    //   shall prevent regressions in that area. Testing parseNumberTriplet() alone would not be
    //   sufficient because the results depend on the way both functions interact.

    QTest::addColumn<QString>("type");
    QTest::addColumn<QString>("values");
    QTest::addColumn<bool>("isValid");
    QTest::addColumn<QSvgAnimatedPropertyTransform::TransformComponent::Type>("expectedType");
    QTest::addColumn<QList<QList<qreal>>>("expectedComponents");

    QTest::newRow("translate") << "translate" << "98.7, 6.5; 1.2, 3.45" << true
                               << QSvgAnimatedPropertyTransform::TransformComponent::Translate
                               << QList<QList<qreal>>{ { 98.7, 6.5 }, { 1.2, 3.45 } };
    QTest::newRow("translate-implicit-ty")
            << "translate" << "4; 3.2" << true
            << QSvgAnimatedPropertyTransform::TransformComponent::Translate
            << QList<QList<qreal>>{ { 4., 0. }, { 3.2, 0. } };
    QTest::newRow("scale") << "scale" << "9.7, 6.4; 3.1, 0.8" << true
                           << QSvgAnimatedPropertyTransform::TransformComponent::Scale
                           << QList<QList<qreal>>{ { 9.7, 6.4 }, { 3.1, 0.8 } };
    QTest::newRow("scale-implicit-sy")
            << "scale" << "3; 4" << true << QSvgAnimatedPropertyTransform::TransformComponent::Scale
            << QList<QList<qreal>>{ { 3., 3. }, { 4., 4. } };
    QTest::newRow("rotate") << "rotate" << "1,2.3,3;9,8,7" << true
                            << QSvgAnimatedPropertyTransform::TransformComponent::Rotate
                            << QList<QList<qreal>>{ { 1, 2.3, 3 }, { 9, 8, 7 } };
    QTest::newRow("skewX") << "skewX" << "10;20" << true
                           << QSvgAnimatedPropertyTransform::TransformComponent::Skew
                           << QList<QList<qreal>>{ { 10, 0 }, { 20, 0 } };
    QTest::newRow("skewY") << "skewY" << "10;20" << true
                           << QSvgAnimatedPropertyTransform::TransformComponent::Skew
                           << QList<QList<qreal>>{ { 0, 10 }, { 0, 20 } };
    QTest::newRow("empty-in-commas")
            << "rotate" << "1,,3;4,5,6" << false
            << QSvgAnimatedPropertyTransform::TransformComponent::Rotate << QList<QList<qreal>>{};
    QTest::newRow("space-in-commas")
            << "rotate" << "1, ,3;4,5,6" << false
            << QSvgAnimatedPropertyTransform::TransformComponent::Rotate << QList<QList<qreal>>{};
    QTest::newRow("empty-after-semicolon")
            << "rotate" << "1,2,3;4,5,6;" << true
            << QSvgAnimatedPropertyTransform::TransformComponent::Rotate
            << QList<QList<qreal>>{ { 1, 2, 3 }, { 4, 5, 6 } };
    QTest::newRow("empty-after-comma")
            << "rotate" << "1,2,3;4,5," << false
            << QSvgAnimatedPropertyTransform::TransformComponent::Rotate << QList<QList<qreal>>{};
    QTest::newRow("space-after-comma")
            << "rotate" << "1,2,3;4,5, " << false
            << QSvgAnimatedPropertyTransform::TransformComponent::Rotate << QList<QList<qreal>>{};

    // The specification is not explicit about this case but we understand that spaces are allowed
    // as separators. Our reasoning is explained in the commit message.
    // If somebody finds a spec which clearly requires a different behavior, please adjust the test.
    QTest::newRow("vectorlike-values-separated-by-spaces")
            << "rotate" << "1 2\t3;4\n5\r6" << true
            << QSvgAnimatedPropertyTransform::TransformComponent::Rotate
            << QList<QList<qreal>>{ { 1, 2, 3 }, { 4, 5, 6 } };
}

void tst_QSvgHandler::testCreateAnimateTransformNode()
{
    QFETCH(QString, type);
    QFETCH(QString, values);
    QFETCH(bool, isValid);
    QFETCH(QSvgAnimatedPropertyTransform::TransformComponent::Type, expectedType);
    QFETCH(QList<QList<qreal>>, expectedComponents);

    QXmlStreamAttributes attributes;
    QSvgHandler handler(QByteArray(), QtSvg::Option::NoOption, QtSvg::AnimatorType::Automatic);
    handler.startElement(QStringLiteral("svg"), attributes); // create a QSvgDocument in the handler

    attributes.append("type", type);
    attributes.append("values", values);

    const auto node = std::unique_ptr<QSvgNode>{createAnimateTransformNode(handler.document(),
                                                                           attributes, &handler)};

    QEXPECT_FAIL("empty-in-commas", "consecutive commas accepted as valid", Abort);
    QEXPECT_FAIL("space-in-commas", "only space in commas accepted as valid", Abort);
    QEXPECT_FAIL("empty-after-comma", "trailing comma accepted as valid", Abort);
    QEXPECT_FAIL("space-after-comma", "comma followed by space only accepted as valid", Abort);
    QCOMPARE(bool(node), isValid);
    if (!node)
        return;
    const auto &properties = static_cast<QSvgAnimateTransform *>(node.get())->properties();
    QCOMPARE(properties.size(), 1);
    QCOMPARE(properties.first()->type(), QSvgAbstractAnimatedProperty::Transform);
    QList<QList<qreal>> readProperty;
    for (const auto &j :
         static_cast<QSvgAnimatedPropertyTransform *>(properties.first())->components()) {
        readProperty << QList(j.values.cbegin(), j.values.cend());
        QCOMPARE(j.type, expectedType);
    }
    QEXPECT_FAIL("scale-implicit-sy", "incorrect default value for sy in scale", Continue);
    QCOMPARE(readProperty, expectedComponents);
}

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
    QStringView listStrV{ listStr };
    QCOMPARE(parseNumbersList(testNullptr ? nullptr : &listStrV), result);
    if (pointerAdvancedToEnd)
        QVERIFY(listStrV.isEmpty());
}

void tst_QSvgHandler::testResolveColor_data()
{
    QTest::addColumn<QString>("colorStr");
    QTest::addColumn<bool>("returnVal");
    QTest::addColumn<QColor>("expectedColor");

    QTest::newRow("rgb-numbers") << "rgb(1,2,3)" << true << QColor{ 1, 2, 3 };
    QTest::newRow("rgb-numbers-invalid") << "rgb(1,x,3)" << false << QColor();
    QTest::newRow("rgb-numbers-too-large") << "rgb(1,256,3)" << false << QColor();
    QTest::newRow("rgb-numbers-illegal-float") << "rgb(1,2.4,3)" << false << QColor();
}

void tst_QSvgHandler::testResolveColor()
{
    QFETCH(QString, colorStr);
    QFETCH(bool, returnVal);
    QFETCH(QColor, expectedColor);

    QSvgHandler handler(QByteArray(), QtSvg::Option::NoOption, QtSvg::AnimatorType::Automatic);
    QColor actualColor;
    QEXPECT_FAIL("rgb-numbers-too-large", "accepted too large value", Continue);
    QEXPECT_FAIL("rgb-numbers-illegal-float", "accepted float where it's not allowed", Continue);
    QCOMPARE(resolveColor(colorStr, actualColor, &handler), returnVal);
    if (returnVal)
        QCOMPARE(actualColor, expectedColor);
}

#endif // QT_BUILD_INTERNAL

QTEST_MAIN(tst_QSvgHandler)
#include "tst_qsvghandler.moc"
