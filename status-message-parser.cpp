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

#include <QTimer>
#include <QElapsedTimer>
#include <QDateTime>
#include <QHash>
#include <QString>
#include <QRegularExpression>

#include "status-message-parser.h"
#include "telepathy-mpris.h"
#include "ktp_kded_debug.h"

static const QList<QString> parserTokens = QList<QString>() << QLatin1String("%tr")
                 << QLatin1String("%te") << QLatin1String("%time") << QLatin1String("%utc")
                 << QLatin1String("%title") << QLatin1String("%artist") << QLatin1String("%album")
                 << QLatin1String("%track") << QLatin1String("%tu") << QLatin1String("%tx")
                 << QLatin1String("%xm") << QLatin1String("%tf") << QLatin1String("%uf")
                 << QLatin1String("%sp") << QLatin1String("%um");

StatusMessageParser::StatusMessageParser(QObject *parent)
    : QObject(parent),
    m_elapsedTimer(new QElapsedTimer()),
    m_updateTimer(new QTimer(this)),
    m_expireTimer(new QTimer(this)),
    m_mpris(new TelepathyMPRIS(this))
{
    clearStatusMessage();

    m_expireTimer->setSingleShot(true);
    connect(m_expireTimer, &QTimer::timeout, [this] {
        Q_EMIT statusMessageChanged(parseStatusMessage(m_followUp));
    });

    connect(m_updateTimer, &QTimer::timeout, [this] {
        updateMessage();
        Q_EMIT statusMessageChanged(m_statusMessage);
    });

    connect(m_mpris, &TelepathyMPRIS::playerChange, [this] {
        if ((m_mpris->player()->playState <= TelepathyMPRIS::Stopped) && m_nowPlayingExpire) {
            parseStatusMessage(m_followUp);
        }

        updateMessage();
        Q_EMIT statusMessageChanged(m_statusMessage);
    });
}

StatusMessageParser::~StatusMessageParser()
{
}

QString StatusMessageParser::statusMessage() const
{
    return m_statusMessage;
}

QHash<QString, QString> StatusMessageParser::tokens() const
{
    return m_tokens;
}

QString StatusMessageParser::parseStatusMessage(QString message)
{
    if (message.isEmpty()) {
        clearStatusMessage();
        return QString();
    } else {
        m_tokens.clear();
    }

    QRegularExpression tokenRegexp(QLatin1String("([\\%|\\\\\\%]+[a-z]+\\+\"([^\"]*|[\"]*)\"*)*(?1)|[\\%|\\\\\\%]+[a-z]+\\+?[\\w\\.]+"));
    QRegularExpressionMatchIterator tokens = tokenRegexp.globalMatch(message);
    while (tokens.hasNext()) {
        QRegularExpressionMatch fullmatch = tokens.next();
        QString token = fullmatch.captured(0).section(QLatin1String("+"), 0, 0);
        QString tokenParam = fullmatch.captured(0).section(QLatin1String("+"), 1);

        if (!parserTokens.contains(token)) {
            qCDebug(KTP_KDED_MODULE) << "token match ignore" << token;
            continue;
        } else if (!tokenParam.isEmpty()) {
            if (tokenParam.startsWith(QLatin1String("\"")) && tokenParam.endsWith(QLatin1String("\""))) {
                tokenParam.remove(0, 1);
                tokenParam.remove(-1, 1);
            }
            qCDebug(KTP_KDED_MODULE) << "token command match" << token << tokenParam;
        } else {
            qCDebug(KTP_KDED_MODULE) << "token match" << token;
        }

        m_tokens.insert(token, tokenParam);

        if ((token == QLatin1String("%tr")) && !tokenParam.isEmpty()) {
            m_expireTimer->stop();
            m_expireTimer->setInterval(tokenParam.toDouble() * 1000 * 60);
            message.replace(fullmatch.captured(0), token);
        }

        if ((token == QLatin1String("%te")) && !tokenParam.isEmpty()) {
            m_elapsedTimer->invalidate();
            m_intervalElapsed = (tokenParam.toDouble() * 1000 * 60);
            message.replace(fullmatch.captured(0), token);
        }

        if ((token == QLatin1String("%time")) && !tokenParam.isEmpty()) {
            m_expireTimer->stop();
            m_expireTimer->setInterval(tokenParam.toDouble() * 1000 * 60);
            message.replace(fullmatch.captured(0), QDateTime::currentDateTime().addMSecs(tokenParam.toDouble() * 1000 * 60).toString(m_timeFormat));
        }

        if ((token == QLatin1String("%utc")) && !tokenParam.isEmpty()) {
            m_expireTimer->stop();
            m_expireTimer->setInterval(tokenParam.toDouble() * 1000 * 60);
            message.replace(fullmatch.captured(0), QDateTime::currentDateTimeUtc().addMSecs(tokenParam.toDouble() * 1000 * 60).toString(m_utcFormat));
        }

        if ((token == QLatin1String("%tu"))) {
            m_updateTimer->stop();
            m_updateTimer->setInterval(tokenParam.toDouble() * 1000 * 60);
            message.remove(fullmatch.captured(0));
        }

        if ((token == QLatin1String("%tx"))) {
            m_nowPlayingExpire = (tokenParam == QLatin1String("np"));
            m_expireTimer->stop();
            m_expireTimer->setInterval(tokenParam.toDouble() * 1000 * 60);
            message.remove(fullmatch.captured(0));
        }

        if ((token == QLatin1String("%xm"))) {
            m_followUp = tokenParam;
            message.remove(fullmatch.captured(0));
        }

        if ((token == QLatin1String("%tf"))) {
            m_timeFormat = tokenParam;
            message.remove(fullmatch.captured(0));
        }

        if ((token == QLatin1String("%uf"))) {
            m_utcFormat = tokenParam;
            message.remove(fullmatch.captured(0));
        }

        if ((token == QLatin1String("%sp"))) {
            m_separator = tokenParam;
            message.remove(fullmatch.captured(0));
        }

        if ((token == QLatin1String("%um"))) {
            message.remove(fullmatch.captured(0));
        }
    }

    if (m_tokens.contains(QLatin1String("%time"))) {
        message.replace(QRegularExpression(QLatin1String("\\B%time\\b")), QDateTime::currentDateTime().toString(m_timeFormat));
    }

    if (m_tokens.contains(QLatin1String("%utc"))) {
        message.replace(QRegularExpression(QLatin1String("\\B%utc\\b")), QDateTime::currentDateTimeUtc().toString(m_utcFormat));
    }

    if ((m_expireTimer->interval() != 0) && !m_expireTimer->isActive()) {
        m_expireTimer->start();
    }

    if (!m_elapsedTimer->isValid() && m_tokens.contains(QLatin1String("%te"))) {
        m_elapsedTimer->start();
    }

    message.replace(QRegularExpression(QLatin1String("\\B\\\\%(?=[a-z]+)")), QLatin1String("%"));

    auto hasMpris = [] (const QString &message) {
        bool hasMpris = message.contains(QRegularExpression(QLatin1String("\\B%title\\b")))
          || message.contains(QRegularExpression(QLatin1String("\\B%artist\\b")))
          || message.contains(QRegularExpression(QLatin1String("\\B%album\\b")))
          || message.contains(QRegularExpression(QLatin1String("\\B%track\\b")));

        return hasMpris;
    };

    if (hasMpris(message) != hasMpris(m_parsedMessage)) {
        m_parsedMessage = message;
        m_mpris->enable(hasMpris(message));
    } else {
        m_parsedMessage = message;
    }

    updateMessage();

    m_updateTimer->start();

    return m_statusMessage;
}

void StatusMessageParser::updateMessage()
{
    QString message = m_parsedMessage;
    QString remaining;
    QString elapsed;
    QString title, artist, album, trackNr;
    title = artist = album = trackNr = m_separator;

    uint remainingTime = std::round(m_expireTimer->remainingTime() / 1000 / 60);
    if (remainingTime < 1) {
        remaining = QLatin1String("<1");
    } else {
        remaining = QString::number(remainingTime);
    }

    uint elapsedTime = std::round((m_elapsedTimer->elapsed() + m_intervalElapsed) / 1000 / 60);
    if (elapsedTime < 1) {
        elapsed = QLatin1String("<1");
    } else {
        elapsed = QString::number(elapsedTime);
    }

    if (m_tokens.contains(QLatin1String("%tr"))) {
        message.replace(QRegularExpression(QLatin1String("%tr")), remaining);
    }

    if (m_tokens.contains(QLatin1String("%te"))) {
        message.replace(QRegularExpression(QLatin1String("%te")), elapsed);
    }

    if (m_mpris->player()->playState == TelepathyMPRIS::Playing) {
        if (!m_mpris->player()->metadata.value(QLatin1String("xesam:title")).toString().isEmpty()) {
            title = m_mpris->player()->metadata.value(QLatin1String("xesam:title")).toString();
        }

        if (!m_mpris->player()->metadata.value(QLatin1String("xesam:artist")).toString().isEmpty()) {
            artist = m_mpris->player()->metadata.value(QLatin1String("xesam:artist")).toString();
        } else if (!m_mpris->player()->metadata.value(QLatin1String("xesam:albumArtist")).toString().isEmpty()) {
            artist = m_mpris->player()->metadata.value(QLatin1String("xesam:albumArtist")).toString();
        }

        if (!m_mpris->player()->metadata.value(QLatin1String("xesam:album")).toString().isEmpty()) {
            album = m_mpris->player()->metadata.value(QLatin1String("xesam:album")).toString();
        }

        if (!m_mpris->player()->metadata.value(QLatin1String("xesam:trackNumber")).toString().isEmpty()) {
            trackNr = m_mpris->player()->metadata.value(QLatin1String("xesam:trackNumber")).toString();
        }
    }

    if (m_tokens.contains(QLatin1String("%title"))) {
        message.replace(QRegularExpression(QLatin1String("\\B(?<![\\\\|%])%title\\b")), title);
    }

    if (m_tokens.contains(QLatin1String("%album"))) {
        message.replace(QRegularExpression(QLatin1String("\\B(?<![\\\\|%])%album\\b")), album);
    }

    if (m_tokens.contains(QLatin1String("%artist"))) {
        message.replace(QRegularExpression(QLatin1String("\\B(?<![\\\\|%])%artist\\b")), artist);
    }

    if (m_tokens.contains(QLatin1String("%track"))) {
        message.replace(QRegularExpression(QLatin1String("\\B(?<![\\\\|%])%track\\b")), trackNr);
    }

    m_statusMessage = message.simplified();
}

void StatusMessageParser::clearStatusMessage()
{
    m_expireTimer->stop();
    m_updateTimer->stop();
    m_updateTimer->setInterval(300000);
    m_elapsedTimer->invalidate();
    m_intervalElapsed = 0;

    m_nowPlayingExpire = false;
    m_mpris->enable(false);

    m_statusMessage.clear();
    m_parsedMessage.clear();
    m_followUp.clear();
    m_tokens.clear();

    m_timeFormat = QLatin1String("h:mm AP t");
    m_utcFormat = QLatin1String("hh:mm t");
    m_separator = QLatin1String("...");
}
