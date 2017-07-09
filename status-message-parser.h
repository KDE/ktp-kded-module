/*
    Telepathy status message parser class
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

#ifndef STATUSMESSAGEPARSER_H
#define STATUSMESSAGEPARSER_H

#include <QObject>

#include <QTimer>
#include <QElapsedTimer>
#include <QDateTime>
#include <QHash>
#include <QString>

#include "telepathy-mpris.h"

/**
 * This class parses status messages for substitution and command matches,
 * substituting tokens for timers and other data sources as necessary.
 */

class StatusMessageParser : public QObject
{
    Q_OBJECT

public:
    explicit StatusMessageParser(QObject *parent = 0);
    ~StatusMessageParser();

    /**
     * \brief Returns a parsed string.
     *
     * \return The parsed status message.
     */
    QString statusMessage() const;

    /**
     * \brief Returns the tokens with parameters (if any) that were used to construct the status mesage.
     *
     * \return A QHash<QString, QString>.
     */
    QHash<QString, QString> tokens() const;

    /**
     * \brief Returns a parsed status message, and activates any data sources
     * needed for periodic updates.
     *
     * \param message The string to parse for tokens.
     *
     * \return The status message.
     */
    QString parseStatusMessage(QString message);

    /**
     * \brief Clear the status message, deactivating all update sources.
     */
    void clearStatusMessage();

Q_SIGNALS:
    void statusMessageChanged(const QString &message);

private:
    void updateMessage();
    QElapsedTimer *m_elapsedTimer;
    QTimer *m_updateTimer;
    QTimer *m_expireTimer;
    TelepathyMPRIS *m_mpris;

    QHash<QString, QString> m_tokens;
    QString m_statusMessage;
    QString m_parsedMessage;
    QString m_followUp;
    QString m_timeFormat;
    QString m_utcFormat;
    QString m_separator;

    double m_intervalElapsed;
    bool m_nowPlayingExpire;
};

#endif // STATUS_MESSAGE_PARSER_H
