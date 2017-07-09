/*
    Copyright (C) 2017  James D. Smith <smithjd15@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "status-message-parser.h"

#include <QtGui/QApplication>
#include <QtTest/QtTest>
#include <QString>

class testParser: public QObject
{
    Q_OBJECT
private Q_SLOTS:

    void tokenMatch();
    void hiddenMatch();
    void escapedToken();
    void commandMatch();
    void hiddenCommand();
    void escapedCommand();
    void commandMatchWithQuotes();
    void hiddenCommandWithQuotes();
    void escapedCommandWithQuotes();
    void nestedCommandWithQuotes();
    void nestedHiddenCommandWithQuotes();
    void escapedNestedCommandWithQuotes();

private:
    StatusMessageParser *parser = new StatusMessageParser();
};

void testParser::tokenMatch()
{
    QString parsed = parser->parseStatusMessage("%utc %time");
    QVERIFY( !parsed.isEmpty() );
    parser->clearStatusMessage();
}

void testParser::hiddenMatch()
{
    QString parsed = parser->parseStatusMessage("%um %sp");
    QVERIFY( parsed.isEmpty() );
    parser->clearStatusMessage();
}

void testParser::escapedToken()
{
    QString parsed = parser->parseStatusMessage("\\%te \\%sp");
    QVERIFY( parsed == ("%te %sp") );
    parser->clearStatusMessage();
}

void testParser::commandMatch()
{
    QString parsed = parser->parseStatusMessage("%te+3 ( %tr+1.5) One Two");
    QVERIFY( parsed == ("3 ( 1) One Two") );
    parser->clearStatusMessage();
}

void testParser::hiddenCommand()
{
    QString parsed = parser->parseStatusMessage("%tx+3 %tu+2");
    QVERIFY( parsed.isEmpty() );
    parser->clearStatusMessage();
}

void testParser::escapedCommand()
{
    QString parsed = parser->parseStatusMessage("\\%te+3 \\%sp+.");
    QVERIFY( parsed == ("%te+3 %sp+.") );
    parser->clearStatusMessage();
}

void testParser::commandMatchWithQuotes()
{
    QString parsed = parser->parseStatusMessage("%te+\"3\" %tr+\"1.5\" One Two");
    QVERIFY( parsed == ("3 1 One Two") );
    parser->clearStatusMessage();
}

void testParser::hiddenCommandWithQuotes()
{
    QString parsed = parser->parseStatusMessage("%tx+\"3\" %xm+\"One Two\"");
    QVERIFY( parsed.isEmpty() );
    parser->clearStatusMessage();
}

void testParser::escapedCommandWithQuotes()
{
    QString parsed = parser->parseStatusMessage("\\%te+\"3\" \\%xm+\"One Two\"");
    QVERIFY( parsed == ("%te+\"3\" %xm+\"One Two\"") );
    parser->clearStatusMessage();
}

void testParser::nestedCommandWithQuotes()
{
    QString parsed = parser->parseStatusMessage("%sp+\". . .\" %xm+\"%te+\"3\"\"");
    QVERIFY( parsed.isEmpty() );
    parser->clearStatusMessage();
}

void testParser::nestedHiddenCommandWithQuotes()
{
    QString parsed = parser->parseStatusMessage("%xm+\"%tx+\"3\"\"");
    QVERIFY( parsed.isEmpty() );
    parser->clearStatusMessage();
}

void testParser::escapedNestedCommandWithQuotes()
{
    QString parsed = parser->parseStatusMessage("\\%sp+\". . .\" \\%xm+\"%te+\"3\"\"");
    QVERIFY( parsed == ("%sp+\". . .\" %xm+\"%te+\"3\"\"") );
    parser->clearStatusMessage();
}

QTEST_MAIN(testParser)
#include "status-message-parser.moc"
