/**
 * @file HospitalStatusWidget.h
 * @brief SMART AEGIS (AERS) Phase 17 - Municipal Hospital Capacity & Resource Monitor Widget.
 */

#ifndef AEGIS_DASHBOARD_HOSPITAL_STATUS_WIDGET_H
#define AEGIS_DASHBOARD_HOSPITAL_STATUS_WIDGET_H

#include "DashboardDataModel.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QFrame>
#include <memory>
#include <unordered_map>

namespace aegis {
namespace dashboard {

/**
 * @brief Displays hospital bed capacity, trauma capabilities, ICU availability, and divert status.
 */
class HospitalStatusWidget : public QWidget {
    Q_OBJECT

public:
    explicit HospitalStatusWidget(std::shared_ptr<DashboardDataModel> dataModel,
                                  QWidget* parent = nullptr);
    ~HospitalStatusWidget() override = default;

    void RefreshData();

private:
    void SetupUi();

    struct HospitalCardWidgets {
        QFrame* cardFrame{nullptr};
        QLabel* lblName{nullptr};
        QLabel* lblTraumaLevel{nullptr};
        QLabel* lblStatusBadge{nullptr};
        QProgressBar* progressIcu{nullptr};
        QLabel* lblIcuText{nullptr};
        QProgressBar* progressEd{nullptr};
        QLabel* lblEdText{nullptr};
        QLabel* lblSpecialty{nullptr};
    };

    std::shared_ptr<DashboardDataModel> m_dataModel;
    std::unordered_map<std::string, HospitalCardWidgets> m_hospitalCards;
};

} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_HOSPITAL_STATUS_WIDGET_H

