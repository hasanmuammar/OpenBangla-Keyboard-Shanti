#pragma once

#include <QString>

class QWidget;

namespace FlatpakIbusIntegration {
bool isApplicable();
QString statusText();
bool canRemove();
void checkAndPrompt(QWidget *parent);
void showManagementDialog(QWidget *parent);
void setupOrRepair(QWidget *parent);
void remove(QWidget *parent);
}
