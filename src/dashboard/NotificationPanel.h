/**
 * @file NotificationPanel.h
 * @brief SMART AEGIS (AERS) Phase 17 - Municipal Banner Alert & Notification Widget.
 */

#ifndef AEGIS_DASHBOARD_NOTIFICATION_PANEL_H
#define AEGIS_DASHBOARD_NOTIFICATION_PANEL_H

#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtCore/QTimer>

namespace aegis {
namespace dashboard {

/**
 * @brief Pop-up banner notification widget for mission-critical alerts with auto-dismiss.
 */
class NotificationPanel : public QFrame {
    Q_OBJECT

public:
    explicit NotificationPanel(QWidget* parent = nullptr);
    ~NotificationPanel() override = default;

    void ShowNotification(const QString& title,
                          const QString& message,
                          const QString& severity = "INFO",
                          int durationMs = 5000);
    void Dismiss();

    bool IsActive() const { return isVisible(); }
    QString GetCurrentTitle() const { return m_currentTitle; }
    QString GetCurrentMessage() const { return m_currentMessage; }
    QString GetCurrentSeverity() const { return m_currentSeverity; }

private slots:
    void OnTimerTimeout();

private:
    void SetupUi();

    QLabel* m_lblIcon{nullptr};
    QLabel* m_lblTitle{nullptr};
    QLabel* m_lblMessage{nullptr};
    QPushButton* m_btnClose{nullptr};
    QTimer* m_timer{nullptr};

    QString m_currentTitle;
    QString m_currentMessage;
    QString m_currentSeverity;
};

} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_NOTIFICATION_PANEL_H

