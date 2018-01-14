/*
    KControl Module for general Telepathy integration configs
    Copyright (C) 2011  Martin Klapetek <martin.klapetek@gmail.com>
    Copyright (C) 2012  Othmane Moustaouda <othmane.moustaouda@gmail.com>

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


#include "telepathy-kded-config.h"
#include "ui_telepathy-kded-config.h"

#include <KPluginFactory>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KLocalizedString>

#include <QUrl>
#include <QDBusMessage>
#include <QDBusConnection>

#include "column-resizer.h"

K_PLUGIN_FACTORY(KCMTelepathyKDEDModuleConfigFactory, registerPlugin<TelepathyKDEDConfig>();)

static const QString KDED_STATUS_MESSAGE_PARSER_WHATSTHIS(
        i18n("<p>Tokens can be used wherever a status message can be set to create a dynamic status message.</p>")
        + i18n("<p><strong>%tr+&lt;val&gt;</strong>: Countdown to 0 from <strong>&lt;val&gt;</strong> minutes. e.g. %tr+30</p>")
        + i18n("<p><strong>%time+[&lt;val&gt;]</strong>: The current local time, or if a value is specified, the local time <strong>&lt;val&gt;</strong> minutes in the future. e.g. %time+10</p>")
        + i18n("<p><strong>%utc+[&lt;val&gt;]</strong>: The current UTC time, or if a value is specified, the UTC time <strong>&lt;val&gt;</strong> minutes into the future. e.g. %utc</p>")
        + i18n("<p><strong>%te+[&lt;val&gt;]</strong>: Time elapsed from message activation. Append an initial elapsed time &quot;&lt;val&gt;&quot; in minutes. e.g. %te+5</p>")
        + i18n("<p><strong>%title</strong>: Now Playing track title.</p>")
        + i18n("<p><strong>%artist</strong>: Now Playing track or album artist.</p>")
        + i18n("<p><strong>%album</strong>: Now Playing album.</p>")
        + i18n("<p><strong>%track</strong>: Now Playing track number.</p>")
        + i18n("<p><strong>%um+[&lt;val&gt;]</strong>: When specified globally or in an account presence status message, overrides all automatic presence messages. When specified in an automatic presence status message, is substituted for the global or account presence status message (if specified). When <strong>val = g</strong> in an account presence status message or an automatic presence status message, overrides the account presence status message or automatic presence status message with the global presence status message. e.g. %um, %um+g</p>")
        + i18n("<p><strong>%tu+&lt;val&gt;</strong>: Refresh the status message every <strong>&lt;val&gt;</strong> minutes. e.g. %tu+2</p>")
        + i18n("<p><strong>%tx+&lt;val&gt;</strong>: Expire the status message after <strong>&lt;val&gt;</strong> minutes, or when the Now Playing active player stops (<strong>val = np</strong>). e.g. %tx+20, %tx+np</p>")
        + i18n("<p><strong>%xm+&quot;&lt;val&gt;&quot;</strong>: Specify a message to follow %tr, %time, %utc, and %tx token expiry. e.g. %xm+&quot;Back %time. %tx+3 %xm+&quot;Running late&quot;&quot;</p>")
        + i18n("<p><strong>%tf+&quot;&lt;val&gt;&quot;</strong>: Specify the format for local time using QDateTime::toString() expressions. e.g. %tf+&quot;h:mm AP t&quot;</p>")
        + i18n("<p><strong>%uf+&quot;&lt;val&gt;&quot;</strong>: Specify the format for UTC time using QDateTime::toString() expressions. e.g. %uf+&quot;hh:mm t&quot;</p>")
        + i18n("<p><strong>%sp+&quot;&lt;val&gt;&quot;</strong>: Change the separator for empty fields. e.g. %sp+&quot;-&quot;</p>")
        + i18n("<p>Using tokens requires the Telepathy KDED module to be loaded. Tokens can be escaped by prepending a backslash character, e.g. &#92;%sp</p>")
        );

TelepathyKDEDConfig::TelepathyKDEDConfig(QWidget *parent, const QVariantList& args)
    : KCModule(parent, args),
      ui(new Ui::TelepathyKDEDUi())
{
    ui->setupUi(this);

    ColumnResizer *resizer = new ColumnResizer(this);
    resizer->addWidgetsFromLayout(ui->incomingFilesGroupBox->layout(), 0);
    resizer->addWidgetsFromLayout(ui->autoAwayGroupBox->layout(), 0);
    resizer->addWidgetsFromLayout(ui->presenceGroupBox->layout(), 0);

    //TODO enable this when it is supported by the approver
    ui->m_autoAcceptCheckBox->setHidden(true);

    //FIXME: figure out how to use i18ncp without argument for suffix
    ui->m_awayMins->setSuffix(i18nc("Unit after number in spinbox, denotes time unit 'minutes', keep the leading whitespace!",
                                     " minutes"));

    ui->m_xaMins->setSuffix(i18nc("Unit after number in spinbox, denotes time unit 'minutes', keep the leading whitespace!",
                                    " minutes"));

    ui->m_awayMessage->setPlaceholderText(i18n("Leave empty for no message"));
    ui->m_xaMessage->setPlaceholderText(i18n("Leave empty for no message"));

    ui->m_screenSaverAwayMessage->setPlaceholderText(i18n("Leave empty for no message"));

    connect(ui->m_downloadUrlRequester, SIGNAL(textChanged(QString)),
            this, SLOT(settingsHasChanged()));
    connect(ui->m_autoAcceptCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(settingsHasChanged()));
    connect(ui->m_awayCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(settingsHasChanged()));
    connect(ui->m_xaCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(settingsHasChanged()));
    connect(ui->m_awayMins, SIGNAL(valueChanged(int)),
            this, SLOT(settingsHasChanged()));
    connect(ui->m_xaMins, SIGNAL(valueChanged(int)),
            this, SLOT(settingsHasChanged()));
    connect(ui->m_awayMessage, SIGNAL(textChanged(QString)),
            this, SLOT(settingsHasChanged()));
    connect(ui->m_xaMessage, SIGNAL(textChanged(QString)),
            this, SLOT(settingsHasChanged()));
    connect(ui->m_autoConnectCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(settingsHasChanged()));
    connect(ui->m_autoOfflineCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(settingsHasChanged()));
    connect(ui->m_screenSaverAwayCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(settingsHasChanged()));
    connect(ui->m_screenSaverAwayMessage, SIGNAL(textChanged(QString)),
            this, SLOT(settingsHasChanged()));
    connect(ui->m_downloadUrlCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(downloadUrlCheckBoxChanged(bool)));

    connect(ui->m_awayCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(autoAwayChecked(bool)));
    connect(ui->m_xaCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(autoXAChecked(bool)));
    connect(ui->m_autoOfflineCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(autoOfflineChecked(bool)));
    connect(ui->m_screenSaverAwayCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(screenSaverAwayChecked(bool)));
}

TelepathyKDEDConfig::~TelepathyKDEDConfig()
{

}

void TelepathyKDEDConfig::load()
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QLatin1String("ktelepathyrc"));

    // File transfers config
    KConfigGroup filetransferConfig = config->group(QLatin1String("File Transfers"));

    // download directory
    QString downloadDirectory = filetransferConfig.readPathEntry(QLatin1String("downloadDirectory"),
                    QDir::homePath() + QLatin1String("/") + i18nc("This is the download directory in user's home", "Downloads"));
    ui->m_downloadUrlRequester->setUrl(QUrl::fromUserInput(downloadDirectory));
    ui->m_downloadUrlCheckBox->setChecked(filetransferConfig.readEntry(QLatin1String("alwaysAsk"), false));
    ui->m_downloadUrlRequester->setEnabled(!ui->m_downloadUrlCheckBox->isChecked());

    // check if auto-accept file transfers is enabled
    bool autoAcceptEnabled = filetransferConfig.readEntry(QLatin1String("autoAccept"), false);
    ui->m_autoAcceptCheckBox->setChecked(autoAcceptEnabled);

    // KDED module config
    KConfigGroup kdedConfig = config->group("KDED");

    //check if auto-away is enabled
    bool autoAwayEnabled = kdedConfig.readEntry(QLatin1String("autoAwayEnabled"), true);

    //default away time is 5 minutes
    int awayTime = kdedConfig.readEntry(QLatin1String("awayAfter"), 5);

    QString awayMessage = kdedConfig.readEntry(QLatin1String("awayMessage"), QString());

    ui->m_awayCheckBox->setChecked(autoAwayEnabled);
    ui->m_awayMins->setValue(awayTime);

    ui->m_awayMessage->setText(awayMessage);
    ui->m_awayMessage->setWhatsThis(KDED_STATUS_MESSAGE_PARSER_WHATSTHIS);
    enableAwayWidgets(autoAwayEnabled);

    //check for x-away
    bool autoXAEnabled = kdedConfig.readEntry(QLatin1String("autoXAEnabled"), true);

    //default x-away time is 15 minutes
    int xaTime = kdedConfig.readEntry(QLatin1String("xaAfter"), 15);

    QString xaMessage = kdedConfig.readEntry(QLatin1String("xaMessage"), QString());

    //enable auto-x-away only if auto-away is enabled
    ui->m_xaCheckBox->setEnabled(autoAwayEnabled);
    ui->m_xaCheckBox->setChecked(autoXAEnabled && autoAwayEnabled);
    ui->m_xaMins->setValue(xaTime);
    ui->m_xaMessage->setText(xaMessage);
    ui->m_xaMessage->setWhatsThis(KDED_STATUS_MESSAGE_PARSER_WHATSTHIS);
    enableXAWidgets(autoXAEnabled && autoAwayEnabled);

        //check if screen-server-away is enabled
    bool screenSaverAwayEnabled = kdedConfig.readEntry(QLatin1String("screenSaverAwayEnabled"), true);

    QString screenSaverAwayMessage = kdedConfig.readEntry(QLatin1String("screenSaverAwayMessage"), QString());

    ui->m_screenSaverAwayCheckBox->setChecked(screenSaverAwayEnabled);
    ui->m_screenSaverAwayMessage->setText(screenSaverAwayMessage);
    ui->m_screenSaverAwayMessage->setWhatsThis(KDED_STATUS_MESSAGE_PARSER_WHATSTHIS);
    ui->m_screenSaverAwayMessage->setEnabled(screenSaverAwayEnabled);

    // autoconnect
    bool autoConnect = kdedConfig.readEntry(QLatin1String("autoConnect"), false);
    ui->m_autoConnectCheckBox->setChecked(autoConnect);

    KSharedConfigPtr contactListConfig = KSharedConfig::openConfig(QLatin1String("ktpcontactlistrc"));
    KConfigGroup generalConfigGroup(contactListConfig, "General");
    KConfigGroup notifyConfigGroup(contactListConfig, "Notification Messages");

    bool dontCheckForPlasmoid = notifyConfigGroup.readEntry("dont_check_for_plasmoid", false);
    if (dontCheckForPlasmoid) {
        bool shouldGoOffline = generalConfigGroup.readEntry("go_offline_when_closing", false);
        if (shouldGoOffline == true) {
            ui->m_autoOfflineCheckBox->setChecked(true);
        } else {
            ui->m_autoOfflineCheckBox->setChecked(false);
        }
    } else {
      ui->m_autoOfflineCheckBox->setCheckState(Qt::PartiallyChecked);
    }
}

void TelepathyKDEDConfig::save()
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QLatin1String("ktelepathyrc"));

// File transfers config
    KConfigGroup filetransferConfig = config->group(QLatin1String("File Transfers"));

    filetransferConfig.writeEntry(QLatin1String("downloadDirectory"), ui->m_downloadUrlRequester->url().toLocalFile());
    filetransferConfig.writeEntry(QLatin1String("autoAccept"), ui->m_autoAcceptCheckBox->isChecked());
    filetransferConfig.writeEntry(QLatin1String("alwaysAsk"), ui->m_downloadUrlCheckBox->isChecked());
    filetransferConfig.sync();

// KDED module config
    KConfigGroup kdedConfig = config->group("KDED");

    kdedConfig.writeEntry(QLatin1String("autoAwayEnabled"), ui->m_awayCheckBox->isChecked());
    kdedConfig.writeEntry(QLatin1String("awayAfter"), ui->m_awayMins->value());
    kdedConfig.writeEntry(QLatin1String("awayMessage"), ui->m_awayMessage->text());
    kdedConfig.writeEntry(QLatin1String("autoXAEnabled"), ui->m_xaCheckBox->isChecked());
    kdedConfig.writeEntry(QLatin1String("xaAfter"), ui->m_xaMins->value());
    kdedConfig.writeEntry(QLatin1String("xaMessage"), ui->m_xaMessage->text());
    kdedConfig.writeEntry(QLatin1String("screenSaverAwayEnabled"), ui->m_screenSaverAwayCheckBox->isChecked());
    kdedConfig.writeEntry(QLatin1String("screenSaverAwayMessage"), ui->m_screenSaverAwayMessage->text());
    kdedConfig.writeEntry(QLatin1String("autoConnect"), ui->m_autoConnectCheckBox->isChecked());

    KSharedConfigPtr contactListConfig = KSharedConfig::openConfig(QLatin1String("ktpcontactlistrc"));
    KConfigGroup generalConfigGroup(contactListConfig, "General");
    KConfigGroup notifyConfigGroup(contactListConfig, "Notification Messages");

    if (ui->m_autoOfflineCheckBox->checkState() == Qt::Unchecked) {
            notifyConfigGroup.writeEntry("dont_check_for_plasmoid", true);
            generalConfigGroup.writeEntry("go_offline_when_closing", false);
    } else if (ui->m_autoOfflineCheckBox->checkState() == Qt::Checked) {
            notifyConfigGroup.writeEntry("dont_check_for_plasmoid", true);
            generalConfigGroup.writeEntry("go_offline_when_closing", true);
    }

    generalConfigGroup.sync();
    notifyConfigGroup.sync();

    kdedConfig.sync();

    QDBusMessage message = QDBusMessage::createSignal(QLatin1String("/Telepathy"),
                                                      QLatin1String("org.kde.Telepathy"),
                                                      QLatin1String("settingsChange"));
    QDBusConnection::sessionBus().send(message);
}

void TelepathyKDEDConfig::enableAwayWidgets(bool enable)
{
    ui->m_awayMins->setEnabled(enable);
    ui->m_awayMessage->setEnabled(enable);
    ui->m_awayMessageLabel->setEnabled(enable);
    ui->m_awayMinsLabel->setEnabled(enable);
    ui->m_awayInactivityLabel->setEnabled(enable);
}

void TelepathyKDEDConfig::enableXAWidgets(bool enable)
{
    ui->m_xaMins->setEnabled(enable);
    ui->m_xaMessage->setEnabled(enable);
    ui->m_xaMessageLabel->setEnabled(enable);
    ui->m_xaMinsLabel->setEnabled(enable);
    ui->m_xaInactivityLabel->setEnabled(enable);
}

void TelepathyKDEDConfig::autoAwayChecked(bool checked)
{
    ui->m_xaCheckBox->setEnabled(checked);
    enableXAWidgets(checked && ui->m_xaCheckBox->isChecked());
    enableAwayWidgets(checked);
    Q_EMIT changed(true);
}

void TelepathyKDEDConfig::screenSaverAwayChecked(bool checked)
{
    ui->m_screenSaverAwayMessage->setEnabled(checked);
    ui->m_screenSaverAwayLabel->setEnabled(checked);
    Q_EMIT changed(true);
}

void TelepathyKDEDConfig::autoXAChecked(bool checked)
{
    enableXAWidgets(checked);
    Q_EMIT changed(true);
}

void TelepathyKDEDConfig::settingsHasChanged()
{
    Q_EMIT changed(true);
}

void TelepathyKDEDConfig::autoOfflineChecked(bool checked)
{
    Q_UNUSED(checked)

    ui->m_autoOfflineCheckBox->setTristate(false);
}

void TelepathyKDEDConfig::downloadUrlCheckBoxChanged(bool checked)
{
    ui->m_downloadUrlRequester->setEnabled(!checked);
    Q_EMIT changed(true);
}

#include "telepathy-kded-config.moc"
