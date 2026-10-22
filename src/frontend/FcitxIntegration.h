#pragma once

#include <QString>

class QWidget;

/* Mirror of FlatpakIbusIntegration for the Fcitx5 bundle: the settings
 * dialog and tray show the same kind of integration block, with a
 * first-run prompt, "Set up / repair" and "Remove integration"
 * actions. */
namespace FcitxIntegration {
bool isApplicable();
QString statusText();
bool canRemove();
void checkAndPrompt(QWidget *parent);
void showManagementDialog(QWidget *parent);
void setupOrRepair(QWidget *parent);
void remove(QWidget *parent);
}
