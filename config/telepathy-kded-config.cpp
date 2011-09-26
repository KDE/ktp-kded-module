/*
    KControl Module for general Telepathy integration configs
    Copyright (C) 2011  Martin Klapetek <martin.klapetek@gmail.com>

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
#include <KLocalizedString>

K_PLUGIN_FACTORY(KCMTelepathyKDEDModuleConfigFactory, registerPlugin<TelepathyKDEDConfig>();)
K_EXPORT_PLUGIN(KCMTelepathyKDEDModuleConfigFactory("telepathy_kded_module_config", "kcm_telepathy_kded_module_config"))


TelepathyKDEDConfig::TelepathyKDEDConfig(QWidget *parent, const QVariantList& args)
    : KCModule(KCMTelepathyKDEDModuleConfigFactory::componentData(), parent, args),
      ui(new Ui::TelepathyKDEDUi())
{
    ui->setupUi(this);

    //FIXME: figure out how to use i18ncp without argument for suffix
    ui->m_awayMins->setSuffix(i18nc("Unit after number in spinbox, denotes time unit 'minutes', keep the leading whitespace!",
                                     " minutes"));

    ui->m_xaMins->setSuffix(i18nc("Unit after number in spinbox, denotes time unit 'minutes', keep the leading whitespace!",
                                    " minutes"));

    connect(ui->m_awayCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(autoAwayChecked(bool)));

    connect(ui->m_xaCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(settingsHasChanged()));

    connect(ui->m_awayMins, SIGNAL(valueChanged(int)),
            this, SLOT(settingsHasChanged()));

    connect(ui->m_xaMins, SIGNAL(valueChanged(int)),
            this, SLOT(settingsHasChanged()));

    connect(ui->m_nowPlayingCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(settingsHasChanged()));
}

TelepathyKDEDConfig::~TelepathyKDEDConfig()
{

}

void TelepathyKDEDConfig::load()
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QLatin1String("ktelepathyrc"));
    KConfigGroup kdedConfig = config->group("KDED");

    //check if auto-away is enabled
    bool autoAwayEnabled = kdedConfig.readEntry("autoAwayEnabled", true);

    //default away time is 5 minutes
    int awayTime = kdedConfig.readEntry("awayAfter", 5);

    ui->m_awayCheckBox->setChecked(autoAwayEnabled);
    ui->m_awayMins->setValue(awayTime);
    ui->m_awayMins->setEnabled(autoAwayEnabled);

    //check for x-away
    bool autoXAEnabled = kdedConfig.readEntry("autoXAEnabled", true);

    //default x-away time is 15 minutes
    int xaTime = kdedConfig.readEntry("xaAfter", 15);

    //enable auto-x-away only if auto-away is enabled
    ui->m_xaCheckBox->setChecked(autoXAEnabled && autoAwayEnabled);
    ui->m_xaMins->setValue(xaTime);
    ui->m_xaMins->setEnabled(autoXAEnabled && autoAwayEnabled);

    //check if 'Now playing..' is enabled
    bool nowPlayingEnabled = kdedConfig.readEntry("nowPlayingEnabled", true);

    ui->m_nowPlayingCheckBox->setChecked(nowPlayingEnabled);
}

void TelepathyKDEDConfig::save()
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QLatin1String("ktelepathyrc"));
    KConfigGroup kdedConfig = config->group("KDED");

    kdedConfig.writeEntry("autoAwayEnabled", ui->m_awayCheckBox->isChecked());
    kdedConfig.writeEntry("awayAfter", ui->m_awayMins->value());
    kdedConfig.writeEntry("autoXAEnabled", ui->m_xaCheckBox->isChecked());
    kdedConfig.writeEntry("xaAfter", ui->m_xaMins->value());
    kdedConfig.writeEntry("nowPlayingEnabled", ui->m_nowPlayingCheckBox->isChecked());
    kdedConfig.sync();
}

void TelepathyKDEDConfig::autoAwayChecked(bool checked)
{
    ui->m_xaCheckBox->setEnabled(checked);
    ui->m_xaMins->setEnabled(checked);

    ui->m_awayMins->setEnabled(checked);

    Q_EMIT changed(true);
}

void TelepathyKDEDConfig::settingsHasChanged()
{
    Q_EMIT changed(true);
}
