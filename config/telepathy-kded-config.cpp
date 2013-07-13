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
#include <KSharedConfig>
#include <KLocalizedString>

#include <QDBusMessage>
#include <QDBusConnection>

#include "column-resizer.h"
#include "autoconnect.h"

K_PLUGIN_FACTORY(KCMTelepathyKDEDModuleConfigFactory, registerPlugin<TelepathyKDEDConfig>();)
K_EXPORT_PLUGIN(KCMTelepathyKDEDModuleConfigFactory("kcm_ktp_integration_module", "kded_ktp_integration_module"))


TelepathyKDEDConfig::TelepathyKDEDConfig(QWidget *parent, const QVariantList& args)
    : KCModule(KCMTelepathyKDEDModuleConfigFactory::componentData(), parent, args),
      ui(new Ui::TelepathyKDEDUi())
{
    ui->setupUi(this);

    m_tagNames << QLatin1String("%title") << QLatin1String("%artist") << QLatin1String("%album") << QLatin1String("%track");
    // xgettext: no-c-format
    m_localizedTagNames << i18nc("Title tag in now playing plugin, use one word and keep the '%' character.", "%title")
                        // xgettext: no-c-format
                        << i18nc("Artist tag in now playing plugin, use one word and keep the '%' character.", "%artist")
                        // xgettext: no-c-format
                        << i18nc("Album tag in now playing plugin, use one word and keep the '%' character.", "%album")
                        // xgettext: no-c-format
                        << i18nc("Track number tag in now playing plugin, use one word and keep the '%' character.", "%track");

    QStringList itemsIcons;
    itemsIcons << QLatin1String("view-media-lyrics")   //%title
               << QLatin1String("view-media-artist")   //%artist
               << QLatin1String("view-media-playlist") //%album
               << QLatin1String("mixer-cd");           //%track

    ui->m_tagListWidget->setItemsIcons(itemsIcons);
    ui->m_tagListWidget->setLocalizedTagNames(m_localizedTagNames);
    ui->m_tagListWidget->setupItems(); //populate the list, load items icons and set list's maximum size

    ui->m_nowPlayingText->setLocalizedTagNames(m_localizedTagNames);

    ColumnResizer *resizer = new ColumnResizer(this);
    resizer->addWidgetsFromLayout(ui->incomingFilesGroupBox->layout(), 0);
    resizer->addWidgetsFromLayout(ui->autoAwayGroupBox->layout(), 0);
    resizer->addWidgetsFromLayout(ui->nowPlayingGroupBox->layout(), 0);
    resizer->addWidgetsFromLayout(ui->autoConnectGroupBox->layout(), 0);
    resizer->addWidgetsFromLayout(ui->autoOfflineGroupBox->layout(), 0);

    //TODO enable this when it is supported by the approver
    ui->m_autoAcceptCheckBox->setHidden(true);

    //FIXME: figure out how to use i18ncp without argument for suffix
    ui->m_awayMins->setSuffix(i18nc("Unit after number in spinbox, denotes time unit 'minutes', keep the leading whitespace!",
                                     " minutes"));

    ui->m_xaMins->setSuffix(i18nc("Unit after number in spinbox, denotes time unit 'minutes', keep the leading whitespace!",
                                    " minutes"));

    ui->m_awayMessage->setClickMessage(i18n("Leave empty for no message"));
    ui->m_awayMessage->setToolTip(i18n("Use %time to insert UTC time of when you went idle"));

    ui->m_xaMessage->setClickMessage(i18n("Leave empty for no message"));
    ui->m_xaMessage->setToolTip(i18n("Use %time to insert UTC time of when you went idle"));

    ui->m_screenSaverAwayMessage->setClickMessage(i18n("Leave empty for no message"));
    ui->m_screenSaverAwayMessage->setToolTip(i18n("Use %time to insert UTC time of when you went idle"));

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
    connect(ui->m_nowPlayingCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(settingsHasChanged()));
    connect(ui->m_nowPlayingText, SIGNAL(textChanged(QString)),
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
    connect(ui->m_nowPlayingCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(nowPlayingChecked(bool)));
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
    ui->m_downloadUrlRequester->setUrl(KUrl(downloadDirectory));
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
    enableXAWidgets(autoXAEnabled && autoAwayEnabled);

        //check if screen-server-away is enabled
    bool screenSaverAwayEnabled = kdedConfig.readEntry(QLatin1String("screenSaverAwayEnabled"), true);

    QString screenSaverAwayMessage = kdedConfig.readEntry(QLatin1String("screenSaverAwayMessage"), QString());

    ui->m_screenSaverAwayCheckBox->setChecked(screenSaverAwayEnabled);
    ui->m_screenSaverAwayMessage->setText(screenSaverAwayMessage);
    ui->m_screenSaverAwayMessage->setEnabled(screenSaverAwayEnabled);

    //check if 'Now playing..' is enabled
    bool nowPlayingEnabled = kdedConfig.readEntry(QLatin1String("nowPlayingEnabled"), false);
    ui->m_nowPlayingCheckBox->setChecked(nowPlayingEnabled);

    //now playing text
    QString nowPlayingText = kdedConfig.readEntry(QLatin1String("nowPlayingText"),
                                                  i18nc("The default text displayed by now playing plugin. "
                                                        "track title: %1, artist: %2, album: %3",
                                                        "Now listening to %1 by %2 from album %3",
                                                        QLatin1String("%title"), QLatin1String("%artist"), QLatin1String("%album")));

    //in nowPlayingText tag names aren't localized, here they're replaced with the localized ones
    for (int i = 0; i < m_tagNames.size(); i++) {
        nowPlayingText.replace(m_tagNames.at(i), m_localizedTagNames.at(i));
    }

    ui->m_nowPlayingText->setText(nowPlayingText);
    ui->m_nowPlayingText->setEnabled(nowPlayingEnabled);
    ui->m_tagListWidget->setEnabled(nowPlayingEnabled);

    // autoconnect
    QString autoConnectString = kdedConfig.readEntry(QLatin1String("autoConnect"), AutoConnect::modeToString(AutoConnect::Manual));
    AutoConnect::Mode autoConnectMode = AutoConnect::stringToMode(autoConnectString);

    switch (autoConnectMode) {
    case AutoConnect::Disabled:
        ui->m_autoConnectCheckBox->setTristate(false);
        ui->m_autoConnectCheckBox->setChecked(false);
        break;
    case AutoConnect::Enabled:
        ui->m_autoConnectCheckBox->setTristate(false);
        ui->m_autoConnectCheckBox->setChecked(true);
        break;
    case AutoConnect::Manual:
        // AutoConnect::Manual is the default
    default:
        ui->m_autoConnectCheckBox->setTristate(true);
        ui->m_autoConnectCheckBox->setCheckState(Qt::PartiallyChecked);
    }

    KSharedConfigPtr contactListConfig = KSharedConfig::openConfig(QLatin1String("ktp-contactlistrc"));
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
    kdedConfig.writeEntry(QLatin1String("nowPlayingEnabled"), ui->m_nowPlayingCheckBox->isChecked());
    kdedConfig.writeEntry(QLatin1String("screenSaverAwayEnabled"), ui->m_screenSaverAwayCheckBox->isChecked());
    kdedConfig.writeEntry(QLatin1String("screenSaverAwayMessage"), ui->m_screenSaverAwayMessage->text());

    //we store a nowPlayingText version with untranslated tag names
    QString modifiedNowPlayingText = ui->m_nowPlayingText->text();
    for (int i = 0; i < m_tagNames.size(); i++) {
        modifiedNowPlayingText.replace(m_localizedTagNames.at(i), m_tagNames.at(i));
    }

    kdedConfig.writeEntry(QLatin1String("nowPlayingText"), modifiedNowPlayingText);

    switch (ui->m_autoConnectCheckBox->checkState()) {
    case Qt::Unchecked:
        kdedConfig.writeEntry(QLatin1String("autoConnect"), AutoConnect::modeToString(AutoConnect::Disabled));
        break;
    case Qt::PartiallyChecked:
        kdedConfig.writeEntry(QLatin1String("autoConnect"), AutoConnect::modeToString(AutoConnect::Manual));
        break;
    case Qt::Checked:
        kdedConfig.writeEntry(QLatin1String("autoConnect"), AutoConnect::modeToString(AutoConnect::Enabled));
    }

    KSharedConfigPtr contactListConfig = KSharedConfig::openConfig(QLatin1String("ktp-contactlistrc"));
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

void TelepathyKDEDConfig::nowPlayingChecked(bool checked)
{
    ui->m_nowPlayingText->setEnabled(checked);
    ui->m_tagListWidget->setEnabled(checked);
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
