#pragma once

class QWidget;

namespace FlatpakIbusIntegration {
bool isApplicable();
void checkAndPrompt(QWidget *parent);
void showManagementDialog(QWidget *parent);
}
