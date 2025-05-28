#pragma once

#include "Widget.g.h"
#include <winrt/Windows.UI.Xaml.Navigation.h>
#include <winrt/Microsoft.Gaming.XboxGameBar.h>

// ADLX headers
#include "ADLXHelper.h"
#include "I3DSettings.h"
#include "I3DSettings1.h"
#include "I3DSettings2.h"


namespace winrt::WidgetSettingsSample::implementation
{
    struct Widget : WidgetT<Widget>
    {
        Widget();
        ~Widget();

        // Page life‑cycle
        void OnNavigatedTo(winrt::Windows::UI::Xaml::Navigation::NavigationEventArgs e);
        void OnNavigatedFrom(winrt::Windows::UI::Xaml::Navigation::NavigationEventArgs e);

        // UI handlers
        void btnAfmfClick(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void btnRisClick(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void slRISSharpeningValueChanged(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& args);

        // Settings click handler for widget settings click event
        Windows::Foundation::IAsyncAction SettingsButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::Foundation::IInspectable const& e);
    private:
        // Helper methods
        void InitializeADLX();
        void ReleaseADLX();
        void RefreshUIStates();
        void SetStatus(const winrt::hstring& msg);

        // Xbox Game Bar
        winrt::event_token m_settingsToken{};
        Microsoft::Gaming::XboxGameBar::XboxGameBarWidget m_widget{ nullptr };
        Microsoft::Gaming::XboxGameBar::XboxGameBarWidgetControl m_widgetControl{ nullptr };

        // ADLX
        ADLXHelper m_adlxHelper;
        adlx::IADLXGPUListPtr m_gpuList;
        adlx::IADLXGPUPtr m_gpu;
        adlx::IADLX3DSettingsServicesPtr m_3DServices;
        adlx::IADLX3DSettingsServices1Ptr m_3DServices1;
        adlx::IADLX3DSettingsServices2Ptr m_3DServices2;

        // Feature interfaces
        adlx::IADLX3DAMDFluidMotionFramesPtr m_afmf;
        adlx::IADLX3DImageSharpeningPtr m_imageSharpen;
        adlx::IADLX3DImageSharpenDesktopPtr m_imageSharpenDesktop;
        ADLX_IntRange m_sharpRange{ 0 };

        bool m_adlxReady{ false };
    };
}

namespace winrt::WidgetSettingsSample::factory_implementation
{
    struct Widget : WidgetT<Widget, implementation::Widget>
    {
    };
}